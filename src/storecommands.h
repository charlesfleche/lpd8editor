#ifndef STORECOMMANDS_H
#define STORECOMMANDS_H

#include "program.h"
#include "programstore.h"

#include <QString>
#include <QUndoCommand>

#include <expected>
#include <memory>

namespace lpd8 {

// Undo commands over a ProgramStore (doc/IMPROVEMENTS.md, item 4.6).
//
// Each command holds program ids and values, never rows or pointers into a
// model, so it stays right whatever the views do. If the program it is about
// is gone when it runs (deleted some other way), it does nothing and marks
// itself obsolete: QUndoStack then drops it instead of acting on another
// program.
//
// The make*() functions tell three cases apart, which a model's setData()
// needs (true for a no-op, false for a refusal):
// - a command to push;
// - nullptr: the change would change nothing (the same value). Nothing is
//   pushed, so the stack never holds an empty step;
// - a CommandRefusal: the program doesn't exist, or the value is not
//   acceptable (Program::isValid(), a name that isn't valid UTF-16).
// Like the SQLite commands they replace, they don't merge: each edit is one
// step, and value edits have no undo text, as before ('Undo').

enum class CommandRefusal {
    UnknownProgram,
    InvalidValue,
};

using CommandOrRefusal = std::expected<std::unique_ptr<QUndoCommand>, CommandRefusal>;

// The store must outlive the stack that holds these commands: the stack's
// owner declares it after the store, so it is destroyed first, and clears it
// on programsAboutToBeReset (doc/IMPROVEMENTS.md, item 4.7a).
CommandOrRefusal makeSetProgramCommand(ProgramStore &store, int id, const Program &program);
CommandOrRefusal makeRenameCommand(ProgramStore &store, int id, const QString &name);
CommandOrRefusal makeAddCommand(ProgramStore &store, const QString &name, const Program &program);
CommandOrRefusal makeRemoveCommand(ProgramStore &store, int id);

} // namespace lpd8

#endif // STORECOMMANDS_H
