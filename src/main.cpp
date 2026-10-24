#include "alsaseqtransport.h"
#include "app.h"
#include "app_info.h"
#include "utils.h"

#include <QApplication>
#include <QIcon>
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
    // The installed .desktop file (data/): on Wayland, the compositor finds
    // the window's icon and name through it.
    QGuiApplication::setDesktopFileName(QStringLiteral("io.github.charlesfleche.lpd8editor"));

    app.setProperty("applicationLink", appUrl);
    app.setProperty("applicationLicense", "MIT License");
    app.setProperty("applicationCopyright", "Copyright (c) 2017-2018 Charles Flèche");
    // For every window, the startup questions too. A resource of the
    // executable (src/CMakeLists.txt), so it is set here, not in App.
    QApplication::setWindowIcon(QIcon(":/lpd8editor.svg"));

    lpd8::MessageBoxDialogs dialogs;
    lpd8::App lpd8editor(lpd8::App::Options{
            .sqlitePath = defaultDbPath(),
            .storePath = defaultStorePath(),
            .dialogs = &dialogs,
            // Without a sequencer (no /dev/snd/seq), the application still
            // edits programs and files.
            .makeTransport = []() -> std::unique_ptr<lpd8::MidiTransport> {
                try {
                    return std::make_unique<lpd8::AlsaSeqTransport>();
                } catch (const std::runtime_error &e) {
                    qWarning() << "Disabling MIDI:" << e.what();
                    return nullptr;
                }
            },
    });
    // The user chose Quit at startup: nothing was written (app.h).
    if (!lpd8editor.start()) {
        return EXIT_FAILURE;
    }
    return QApplication::exec();
}
