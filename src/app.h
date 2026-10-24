#ifndef APP_H
#define APP_H

#include "migration.h"
#include "programsdocument.h"

#include <QDateTime>
#include <QList>
#include <QString>

#include <expected>
#include <functional>
#include <memory>
#include <optional>

class MainWindow;
class QWidget;

namespace lpd8 {

class MidiTransport;

// The questions the startup asks, before the event loop runs
// (doc/IMPROVEMENTS.md, item 4.11b). An interface, so the tests answer
// them with a fake and check what was asked. The application's
// implementation is MessageBoxStartupDialogs.
class StartupDialogs
{
public:
    enum class Answer {
        Quit,
        Retry, // run the migration again
        SetAside, // rename the damaged programs file, then start empty
    };

    StartupDialogs() = default;
    virtual ~StartupDialogs() = default;
    StartupDialogs(const StartupDialogs &) = delete;
    StartupDialogs &operator=(const StartupDialogs &) = delete;
    StartupDialogs(StartupDialogs &&) = delete;
    StartupDialogs &operator=(StartupDialogs &&) = delete;

    // Asks the user, with Quit and the answers in 'offered' (which never
    // holds Quit). Quit is the default button, and what Escape picks: a
    // user who doesn't read the question changes nothing.
    virtual Answer ask(const QString &text, const QList<Answer> &offered) = 0;
    // Tells the user something, over 'parent' (nullptr before the window
    // exists). Only one button: there is nothing to choose.
    virtual void tell(QWidget *parent, const QString &text) = 0;
};

// The StartupDialogs of the application: modal message boxes.
class MessageBoxStartupDialogs : public StartupDialogs
{
public:
    Answer ask(const QString &text, const QList<Answer> &offered) override;
    void tell(QWidget *parent, const QString &text) override;
};

// What App needs of the session manager when the session ends (logging
// out), behind an interface: a test can't create a QSessionManager, so it
// plays one with a fake. The application adapts QSessionManager.
class SessionEnd
{
public:
    SessionEnd() = default;
    virtual ~SessionEnd() = default;
    SessionEnd(const SessionEnd &) = delete;
    SessionEnd &operator=(const SessionEnd &) = delete;
    SessionEnd(SessionEnd &&) = delete;
    SessionEnd &operator=(SessionEnd &&) = delete;

    // True if the user may be asked something now. It may wait until the
    // session manager allows it.
    virtual bool allowsInteraction() = 0;
    // Done asking: the session manager may go on.
    virtual void release() = 0;
};

// The composition root (doc/ARCHITECTURE.md, "App"): it runs the startup,
// then owns what the application is made of. main() is QApplication, App,
// start(), exec().
//
// Members are declared in the order they are created, the document, the
// transport, then the window, so they are destroyed in the reverse order:
// the window first, while what it uses still exists.
class App
{
public:
    using Migrate = std::function<std::expected<std::optional<MigrationReport>, MigrationError>(
            const QString &sqlitePath, const QString &jsonPath)>;
    using MakeTransport = std::function<std::unique_ptr<MidiTransport>()>;
    using Clock = std::function<QDateTime()>;

    // What the startup depends on, injected so a test can control it.
    struct Options
    {
        QString sqlitePath; // the old database, migrated once
        QString storePath; // the programs file
        // Not owned: it must outlive start(). Only used during start().
        StartupDialogs *dialogs{nullptr};
        // Creates the MIDI transport once the programs are open, or returns
        // nullptr to turn MIDI off (no sequencer).
        MakeTransport makeTransport;
        // The time that names a set-aside file: a test picks it, to take
        // the name in advance.
        Clock now = [] { return QDateTime::currentDateTime(); };
        // The migration: a test replaces it to make the load fail right
        // after a migration.
        Migrate migrate = migrateSqliteDatabase;
    };

    explicit App(Options options);
    ~App();
    App(const App &) = delete;
    App &operator=(const App &) = delete;
    App(App &&) = delete;
    App &operator=(App &&) = delete;

    // Migrates the old database if needed, loads the programs, asks the
    // user what to do if either fails, and shows the window. Returns false
    // if the user chose Quit: the caller returns without running the event
    // loop. Apart from the migration, nothing is written unless the user
    // chose an action that writes (setting the file aside, a rename).
    bool start();

    // The session ends: save now, since the application may be killed
    // without quitting. Connected to QGuiApplication::commitDataRequest by
    // start(), through an adapter; public so a test can call it. Item
    // 4.11c uses 'session' to report a failed save only when the user may
    // be asked.
    void commitData(SessionEnd &session);

    // For the tests.
    ProgramsDocument &document() { return m_document; }
    // nullptr until start() succeeds.
    MainWindow *window() const { return m_window.get(); }

private:
    // Each returns false if the user chose Quit.
    bool migrate();
    bool load();

    Options m_options;
    // A report the window must show, once it exists.
    std::optional<MigrationReport> m_report;

    // Order matters: see the class comment.
    ProgramsDocument m_document;
    std::unique_ptr<MidiTransport> m_transport;
    std::unique_ptr<MainWindow> m_window;
};

} // namespace lpd8

#endif // APP_H
