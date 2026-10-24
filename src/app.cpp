#include "app.h"

#include "lpd8device.h"
#include "mainwindow.h"
#include "miditransport.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>
#include <QPushButton>
#include <QSessionManager>

#include <utility>

namespace lpd8 {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("App", text);
}

QString native(const QString &path)
{
    return QDir::toNativeSeparators(path);
}

// What the user can do about a programs file that can't be loaded, by
// kind. Only a damaged file may be set aside (programstore.h).
QString loadAdvice(LoadError::Kind kind)
{
    switch (kind) {
    case LoadError::Kind::Damaged:
        return tr("The file may be damaged. You can repair it in a text editor and start "
                  "lpd8editor again, or set it aside: it is renamed, not deleted, and "
                  "lpd8editor starts with no programs.");
    case LoadError::Kind::NewerFormat:
        return tr("Use the newer version of lpd8editor that wrote it: this version can't "
                  "open it, and leaves it as it is.");
    case LoadError::Kind::CannotRead:
        break;
    }
    return tr("Check that you can read the file, and the folders above it, then start "
              "lpd8editor again.");
}

// QSessionManager behind SessionEnd. Not owned: it lives for the duration
// of the commitDataRequest signal.
class QtSessionEnd : public SessionEnd
{
public:
    explicit QtSessionEnd(QSessionManager &manager)
        : m_manager(&manager)
    { }

    bool allowsInteraction() override { return m_manager->allowsInteraction(); }
    void release() override { m_manager->release(); }

private:
    QSessionManager *m_manager;
};

} // namespace

AppDialogs::Answer MessageBoxDialogs::ask(const QString &text, const QList<Answer> &offered)
{
    QMessageBox box(QMessageBox::Warning, QApplication::applicationName(), text);
    QPushButton *quit = box.addButton(tr("Quit"), QMessageBox::RejectRole);
    QList<std::pair<QAbstractButton *, Answer>> buttons{{quit, Answer::Quit}};
    for (const Answer answer : offered) {
        switch (answer) {
        case Answer::Retry:
            buttons.append({box.addButton(QMessageBox::Retry), answer});
            break;
        case Answer::SetAside:
            buttons.append({box.addButton(tr("Set Aside"), QMessageBox::AcceptRole), answer});
            break;
        case Answer::Quit:
            break; // always there
        }
    }
    box.setDefaultButton(quit);
    box.setEscapeButton(quit);
    box.exec();
    for (const auto &[button, answer] : buttons) {
        if (box.clickedButton() == button) {
            return answer;
        }
    }
    return Answer::Quit; // closed some other way
}

void MessageBoxDialogs::tell(QWidget *parent, const QString &text)
{
    QMessageBox::information(parent, QApplication::applicationName(), text);
}

bool MessageBoxDialogs::confirmCloseWithoutSaving(QWidget *parent, const QString &text)
{
    // Over the window, which is still there: the close was only asked for.
    QMessageBox box(QMessageBox::Warning,
                    QApplication::applicationName(),
                    text,
                    QMessageBox::NoButton,
                    parent);
    const QPushButton *close =
            box.addButton(tr("Close Without Saving"), QMessageBox::DestructiveRole);
    QAbstractButton *cancel = box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(qobject_cast<QPushButton *>(cancel));
    box.setEscapeButton(cancel);
    box.exec();
    return box.clickedButton() == close;
}

App::App(Options options)
    : m_options(std::move(options))
    , m_document(m_options.storePath)
{
    Q_ASSERT(m_options.dialogs != nullptr);
}

App::~App()
{
    // The handler shows problems in the window, which goes first. Nothing
    // can call it during the teardown today (no events run, and the
    // document's timer stops with it), but in case a future teardown runs
    // events, it must not reach a window that is gone. The members are
    // then destroyed in reverse order: the window first.
    m_document.setSaveFailureHandler({});
}

bool App::start()
{
    if (!migrate() || !load()) {
        return false;
    }

    // Every way out saves: the document doesn't save when it is destroyed.
    // Closing the window saves (its close guard, closeWindow()). The Quit
    // action and closing the last window end in quit(), hence aboutToQuit.
    // The session manager may end the application without it (logging
    // out), so it asks for the data first with commitDataRequest. The
    // document is the connections' context: they end with the App.
    QObject::connect(QCoreApplication::instance(),
                     &QCoreApplication::aboutToQuit,
                     &m_document,
                     [this] { quitting(); });
    if (auto *gui = qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        QObject::connect(gui,
                         &QGuiApplication::commitDataRequest,
                         &m_document,
                         [this](QSessionManager &manager) {
                             QtSessionEnd session(manager);
                             commitData(session);
                         });
    }

    // MIDI, created once the programs are open: a refused startup doesn't
    // even open a sequencer client.
    if (m_options.makeTransport) {
        m_transport = m_options.makeTransport();
    }

    if (m_transport) {
        m_device = std::make_unique<Lpd8Device>(*m_transport);
    }

    m_window = std::make_unique<MainWindow>(m_document, m_transport.get(), m_device.get());
    m_window->setWindowTitle(QApplication::applicationName());
    m_window->setCloseGuard([this] { return closeWindow(); });
    // A save made by the timer, after a change, that fails: shown in the
    // window's box, without blocking the editing. The document reports it
    // once, until a save succeeds again.
    m_document.setSaveFailureHandler([this](const QString &error) {
        m_window->showProblem(tr("%1\n\nYour changes are not saved yet. lpd8editor will try "
                                 "again at the next change, and when the window closes.")
                                      .arg(error));
    });
    m_window->show();

    // The migration report, shown once, over the window. It is also saved
    // next to the old database (migration.h), and the text says where.
    if (m_report) {
        m_options.dialogs->tell(m_window.get(), describe(*m_report));
    }
    return true;
}

bool App::closeWindow()
{
    const auto saved = m_document.saveFinal();
    if (saved) {
        return true;
    }
    if (m_sessionEndsSilently) {
        // The session manager ends the session and forbids asking: Qt
        // turns that into a quit, which closes the window. A question here
        // would stall the logout.
        logLostChanges(saved.error());
        return true;
    }
    // The window is still there and the close can be cancelled: ask. On
    // Cancel the changes stay, unsaved, and the next close tries again.
    if (!m_options.dialogs->confirmCloseWithoutSaving(
                m_window.get(),
                tr("%1\n\nYour changes could not be saved, and are lost if you close the "
                   "window now. Cancel keeps it open: you can make room on the disk, or "
                   "fix the folder's permissions, then close it again.")
                        .arg(saved.error()))) {
        return false;
    }
    m_closedWithoutSaving = true;
    return true;
}

void App::quitting()
{
    if (m_closedWithoutSaving) {
        return; // the user chose to lose the changes, and was told
    }
    const auto saved = m_document.saveFinal();
    if (!saved && m_sessionEndsSilently) {
        logLostChanges(saved.error()); // see closeWindow()
    } else if (!saved) {
        // The application ends whatever the answer: no question, only
        // what happened, modally, so it is seen before the end.
        m_options.dialogs->tell(
                nullptr,
                tr("%1\n\nYour last changes could not be saved, and are lost.").arg(saved.error()));
    }
}

void App::commitData(SessionEnd &session)
{
    const auto saved = m_document.saveFinal();
    if (saved) {
        return;
    }
    // The session manager may forbid asking anything (a logout that must
    // not wait for the user). Then the failure can only be logged.
    if (!session.allowsInteraction()) {
        logLostChanges(saved.error());
        // And so must the close and the quit that follow: nothing more is
        // asked or shown until the application ends.
        m_sessionEndsSilently = true;
        return;
    }
    m_options.dialogs->tell(nullptr,
                            tr("%1\n\nYour last changes could not be saved: if the session "
                               "ends now, they are lost.")
                                    .arg(saved.error()));
    // Done asking: the session manager may go on.
    session.release();
}

void App::logLostChanges(const QString &error)
{
    qCritical("Cannot save the programs at the end of the session: %s", qUtf8Printable(error));
}

bool App::migrate()
{
    for (;;) {
        auto migrated = m_options.migrate(m_options.sqlitePath, m_options.storePath);
        if (migrated) {
            m_report = std::move(*migrated);
            return true;
        }
        // The migration writes nothing when it fails (migration.h). Never
        // "start without migrating": the first edit would write the
        // programs file, and the migration would never run again.
        const MigrationError &error = migrated.error();
        const QString what = tr("%1\n\nNothing was changed: your programs are still in %2.")
                                     .arg(error.message, native(m_options.sqlitePath));
        QString advice;
        if (error.kind == MigrationError::Kind::HotJournal) {
            advice = tr("A program stopped while writing to it (a crash, or a power cut), and "
                        "SQLite left %1 next to it to undo that write. lpd8editor only reads "
                        "the database, so it can't do this itself. Open the database once with "
                        "the sqlite3 tool (for example: sqlite3 %2 \"pragma integrity_check\"), "
                        "which undoes the interrupted write, then choose Retry. Don't delete "
                        "%1: the database could be left damaged.")
                             .arg(native(error.journalPath), native(m_options.sqlitePath));
        } else {
            advice = tr("If another program is using that file, close it, then choose Retry. If "
                        "the file is damaged, you can repair it with SQLite tools, or move it "
                        "elsewhere to start with no programs.");
        }
        const auto answer = m_options.dialogs->ask(what + QLatin1Char(' ') + advice,
                                                   {AppDialogs::Answer::Retry});
        if (answer != AppDialogs::Answer::Retry) {
            return false;
        }
    }
}

bool App::load()
{
    ProgramStore &store = m_document.store();
    const auto loaded = store.load();
    if (loaded) {
        return true;
    }
    // A failed load leaves the store unwritable (programstore.h): even a
    // save by mistake can't replace the file.
    const LoadError &error = loaded.error();
    const QString advice = loadAdvice(error.kind);
    QString text;
    if (!m_report) {
        text = tr("%1\n\nNothing was changed. %2").arg(error.message, advice);
    } else {
        // The migration did write that file, and moved the old database:
        // say so, with its report, which no later start will show again.
        text = tr("%1\n\nThis file was just created from your old database:\n\n%2\n\n%3")
                       .arg(error.message, describe(*m_report), advice);
        // Shown here: not again over the window.
        m_report.reset();
    }

    if (error.kind != LoadError::Kind::Damaged) {
        m_options.dialogs->ask(text, {});
        return false;
    }
    if (m_options.dialogs->ask(text, {AppDialogs::Answer::SetAside})
        != AppDialogs::Answer::SetAside) {
        return false;
    }
    const auto aside = store.setAsideUnreadableFile(m_options.now());
    if (!aside) {
        // The rename failed: the file is where it was, and nothing else is
        // left to try. Quit only.
        m_options.dialogs->ask(tr("%1\n\nNothing was changed.").arg(aside.error()), {});
        return false;
    }
    m_options.dialogs->tell(nullptr,
                            tr("The programs file was renamed to %1, and lpd8editor starts "
                               "with no programs. Your old programs are still in that file.")
                                    .arg(native(*aside)));
    return true;
}

} // namespace lpd8
