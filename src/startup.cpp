#include "startup.h"

#include "programsdocument.h"

#include <QCoreApplication>
#include <QDir>

namespace lpd8 {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("Startup", text);
}

QString native(const QString &path)
{
    return QDir::toNativeSeparators(path);
}

} // namespace

std::expected<std::optional<QString>, QString> openPrograms(ProgramsDocument &document,
                                                            const QString &sqlitePath)
{
    const auto migrated = migrateSqliteDatabase(sqlitePath, document.store().path());
    if (!migrated) {
        // The migration writes nothing when it fails (migration.h).
        return std::unexpected(
                tr("%1\n\nNothing was changed: your programs are still in %2. If another "
                   "program is using that file, close it and start lpd8editor again. If the "
                   "file is damaged, you can repair it with SQLite tools, or move it "
                   "elsewhere to start with no programs.")
                        .arg(migrated.error(), native(sqlitePath)));
    }
    return loadPrograms(document, *migrated);
}

std::expected<std::optional<QString>, QString>
loadPrograms(ProgramsDocument &document, const std::optional<MigrationReport> &migration)
{
    if (const auto loaded = document.store().load(); !loaded) {
        // A failed load leaves the store unwritable (programstore.h): even
        // a save by mistake can't replace the file.
        const QString advice =
                tr("If the file was written by a newer version of lpd8editor, use that "
                   "version. Otherwise the file may be damaged: you can repair it in a text "
                   "editor, or move it elsewhere to start with no programs. It is at %1.")
                        .arg(native(document.store().path()));
        if (!migration) {
            return std::unexpected(tr("%1\n\nNothing was changed. %2").arg(loaded.error(), advice));
        }
        // The migration did write that file, and moved the old database:
        // say so, with its report, which no later start will show again.
        return std::unexpected(tr("%1\n\nThis file was just created from your old "
                                  "database:\n\n%2\n\n%3")
                                       .arg(loaded.error(), describe(*migration), advice));
    }

    if (migration) {
        return describe(*migration);
    }
    return std::nullopt;
}

} // namespace lpd8
