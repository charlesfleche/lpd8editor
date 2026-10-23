#ifndef STARTUP_H
#define STARTUP_H

#include "migration.h"

#include <QString>

#include <expected>
#include <optional>

namespace lpd8 {

class ProgramsDocument;

// Gets the user's programs into 'document' at startup (item 4.7a,
// doc/IMPROVEMENTS.md): migrates the old database at 'sqlitePath' into the
// document's file if that hasn't been done yet, then loads the file with
// loadPrograms().
//
// - Returns a notice for the user (the migration report, describe()'d) if
//   there is one, or nothing.
// - Returns an error, for the user, if the old database can't be migrated
//   or the file can't be loaded. The caller shows the message and quits.
//   Starting with no programs instead would write a new file, after which
//   the migration would never run again and the old programs would be
//   stranded. The message names the file, the reason, and what the user
//   can do.
std::expected<std::optional<QString>, QString> openPrograms(ProgramsDocument &document,
                                                            const QString &sqlitePath);

// The second step of openPrograms(): loads the document's file, after the
// migration described by 'migration' if one just ran. Separate so that a
// test can make the load fail after a migration.
//
// If the load fails, the message says what was done before: with no
// migration, nothing was written; after one, the report is part of the
// message, since the migration won't run again to show it.
std::expected<std::optional<QString>, QString>
loadPrograms(ProgramsDocument &document, const std::optional<MigrationReport> &migration);

} // namespace lpd8

#endif // STARTUP_H
