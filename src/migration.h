#ifndef MIGRATION_H
#define MIGRATION_H

#include <QString>
#include <QStringList>

#include <expected>
#include <optional>

namespace lpd8 {

// What a migration did, for the user.
struct MigrationReport
{
    int migrated{0}; // programs now in the JSON file
    QStringList skipped; // one line per program left out, saying why
    QString oldFileNowAt; // where db.sqlite3 was moved, or empty if it wasn't
    QString renameError; // why it couldn't be moved, if it couldn't
    QString reportFileAt; // where this report was saved, or empty if it wasn't
    QString reportFileError; // why it couldn't be saved, if it couldn't
};

// The report as text for the user: how many programs moved, those left out
// and why, where the old database is now, and where this text is saved.
QString describe(const MigrationReport &report);

// One-time migration of the programs from the SQLite database used until
// now (db.sqlite3) to the JSON file of ProgramStore. Item 4.4,
// doc/ARCHITECTURE.md.
//
// - Runs only if 'jsonPath' doesn't exist and 'sqlitePath' does. Otherwise
//   it touches neither file and returns nothing (std::nullopt): there is
//   nothing to migrate, or it was done already, maybe by a run that
//   stopped before renaming the old file.
// - Reads the database read-only, through its own named connection.
// - Writes the JSON file atomically, and only after that renames the
//   database to '<file>.migrated' (or '.migrated-<timestamp>' if that name,
//   or its report's, is taken). Just before the rename, it saves the report, describe()'d,
//   next to the backup as '<backup>.txt': the report is shown once, but the
//   skipped programs only live in the backup, so a crash before the user
//   reads it must not lose the list. If the rename then fails, the file is
//   written again to say so. It never deletes anything: if the program stops in between,
//   the JSON file exists, so the next start doesn't migrate again, and the
//   database is still there.
// - Keeps each program's id, name and order. A program that can't be
//   migrated (a value out of range, which databases created by v0.0.0 to
//   v0.0.5 can hold since their tables have no CHECK constraints, or
//   missing rows) is left out and reported in 'skipped', by program and
//   field. Its data stays in the renamed database.
// - Returns an error, and writes nothing, if the database can't be read
//   at all, or if the JSON file can't be written. That includes a database
//   left with a 'hot journal' (db.sqlite3-journal, after a crash during a
//   write): a read-only connection can't roll it back, so SQLite refuses it.
//   The next start then fails the same way, so the user must be told
//   (doc/IMPROVEMENTS.md, item 4.11).
std::expected<std::optional<MigrationReport>, QString>
migrateSqliteDatabase(const QString &sqlitePath, const QString &jsonPath);

} // namespace lpd8

#endif // MIGRATION_H
