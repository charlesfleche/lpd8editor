#include "alsaseqtransport.h"
#include "app_info.h"
#include "mainwindow.h"
#include "programsdocument.h"
#include "startup.h"
#include "utils.h"

#include <QApplication>
#include <QIcon>
#include <QMessageBox>
#include <QSessionManager>
#include <QtDebug>

#include <cstdlib>
#include <memory>
#include <stdexcept>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setOrganizationName(appName);
    QApplication::setApplicationName(appName);
    QApplication::setApplicationVersion(appVersion);

    app.setProperty("applicationLink", appUrl);
    app.setProperty("applicationLicense", "MIT License");
    app.setProperty("applicationCopyright", "Copyright (c) 2017-2018 Charles Flèche");

    // The programs, declared before the window, so they outlive it. They are
    // opened before the window is created: if that fails, the user is told
    // why, and nothing is written (startup.h).
    lpd8::ProgramsDocument document(defaultStorePath());
    const auto opened = lpd8::openPrograms(document, defaultDbPath());
    if (!opened) {
        QMessageBox::critical(nullptr, QApplication::applicationName(), opened.error());
        return EXIT_FAILURE;
    }

    // Every way out saves: the document doesn't save when it is destroyed.
    // Closing the window saves too (MainWindow::closeEvent()). The Quit
    // action and closing the last window end in quit(), hence aboutToQuit.
    // The session manager may end the application without it (logging
    // out), so it asks for the data first with commitDataRequest.
    QObject::connect(
            &app, &QCoreApplication::aboutToQuit, &document, [&document] { document.saveNow(); });
    QObject::connect(&app,
                     &QGuiApplication::commitDataRequest,
                     &document,
                     [&document](QSessionManager &) { document.saveNow(); });

    // MIDI, declared before the window too. Without a sequencer (no
    // /dev/snd/seq), the application still edits programs and files.
    std::unique_ptr<lpd8::MidiTransport> midi;
    try {
        midi = std::make_unique<lpd8::AlsaSeqTransport>();
    } catch (const std::runtime_error &e) {
        qWarning() << "Disabling MIDI:" << e.what();
    }

    MainWindow win(document, midi.get());
    win.setWindowTitle(QApplication::applicationName());
    win.setWindowIcon(QIcon(":/lpd8editor.svg"));
    win.show();

    // The migration report, shown once, over the window. It is also saved
    // next to the old database (migration.h).
    const std::optional<QString> &notice = *opened;
    if (notice) {
        QMessageBox::information(&win, QApplication::applicationName(), *notice);
    }

    return QApplication::exec();
}
