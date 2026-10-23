#include "storecommands.h"

#include <QCoreApplication>

#include <algorithm>

#include <utility>

namespace lpd8 {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("StoreCommands", text);
}

// Through the undo stack, a saved row is always valid (undo and redo run in
// order). If programs were removed some other way in between, it may be past
// the end: the program then goes at the end rather than being lost.
int clampedRow(const ProgramStore &store, int row)
{
    return std::min(row, static_cast<int>(store.programs().size()));
}

class SetProgramCommand : public QUndoCommand
{
public:
    // 'current' is the program as it is now, and 'after' the new value: two
    // different types, so they can't be swapped by mistake (two Programs in
    // a row could, which would silently invert undo and redo). Program is
    // trivially copyable (bytes and arrays), so it is taken by const
    // reference: moving it would copy it anyway.
    SetProgramCommand(ProgramStore &store, const StoredProgram &current, const Program &after)
        : m_store(store)
        , m_id(current.id)
        , m_before(current.program)
        , m_after(after)
    {
        // No undo text, as the SQLite command for value edits had none.
    }

    void redo() override { setObsolete(!m_store.setProgram(m_id, m_after)); }
    void undo() override { setObsolete(!m_store.setProgram(m_id, m_before)); }

private:
    ProgramStore &m_store;
    const int m_id;
    const Program m_before;
    const Program m_after;
};

class RenameCommand : public QUndoCommand
{
public:
    // As SetProgramCommand: the program as it is now, then the new name.
    RenameCommand(ProgramStore &store, const StoredProgram &current, QString after)
        : m_store(store)
        , m_id(current.id)
        , m_before(current.name)
        , m_after(std::move(after))
    {
        // No undo text: a rename was a value edit, which had none.
    }

    void redo() override { setObsolete(!m_store.setName(m_id, m_after)); }
    void undo() override { setObsolete(!m_store.setName(m_id, m_before)); }

private:
    ProgramStore &m_store;
    const int m_id;
    const QString m_before;
    const QString m_after;
};

// Adds a program, and removes it again on undo. The first redo picks a new
// id; later redos (after an undo) reuse it, at the same row, so the program
// comes back exactly as it was.
class AddCommand : public QUndoCommand
{
public:
    AddCommand(ProgramStore &store, QString name, const Program &program)
        : m_store(store)
        , m_name(std::move(name))
        , m_program(program)
    {
        setText(tr("Create program"));
    }

    void redo() override
    {
        const std::optional<int> id = m_id < 0
                ? m_store.add(m_name, m_program)
                : m_store.insert(clampedRow(m_store, m_row), m_name, m_program, m_id);
        if (!id) {
            setObsolete(true);
            return;
        }
        m_id = *id;
        m_row = m_store.rowOf(m_id).value_or(0);
    }

    void undo() override { setObsolete(!m_store.remove(m_id)); }

private:
    ProgramStore &m_store;
    const QString m_name;
    const Program m_program;
    int m_id{-1};
    int m_row{0};
};

// Removes a program, and puts it back on undo, with its id, at its row.
class RemoveCommand : public QUndoCommand
{
public:
    RemoveCommand(ProgramStore &store, int id)
        : m_store(store)
        , m_id(id)
    {
        setText(tr("Delete program"));
    }

    void redo() override
    {
        // Read what to restore at each redo: between an undo and this redo,
        // other programs may have been added or removed, which moves its row.
        const std::optional<StoredProgram> stored = m_store.find(m_id);
        const std::optional<int> row = m_store.rowOf(m_id);
        if (!stored || !row || !m_store.remove(m_id)) {
            setObsolete(true);
            return;
        }
        m_removed = *stored;
        m_row = *row;
    }

    void undo() override
    {
        setObsolete(
                !m_store.insert(clampedRow(m_store, m_row), m_removed.name, m_removed.program, m_id)
                         .has_value());
    }

private:
    ProgramStore &m_store;
    const int m_id;
    StoredProgram m_removed;
    int m_row{0};
};

} // namespace

CommandOrRefusal makeSetProgramCommand(ProgramStore &store, int id, const Program &program)
{
    const std::optional<StoredProgram> stored = store.find(id);
    if (!stored) {
        return std::unexpected(CommandRefusal::UnknownProgram);
    }
    if (!program.isValid()) {
        return std::unexpected(CommandRefusal::InvalidValue);
    }
    if (stored->program == program) {
        return nullptr; // nothing to change
    }
    return std::make_unique<SetProgramCommand>(store, *stored, program);
}

CommandOrRefusal makeRenameCommand(ProgramStore &store, int id, const QString &name)
{
    const std::optional<StoredProgram> stored = store.find(id);
    if (!stored) {
        return std::unexpected(CommandRefusal::UnknownProgram);
    }
    if (!name.isValidUtf16()) {
        return std::unexpected(CommandRefusal::InvalidValue);
    }
    if (stored->name == name) {
        return nullptr; // nothing to change
    }
    return std::make_unique<RenameCommand>(store, *stored, name);
}

CommandOrRefusal makeAddCommand(ProgramStore &store, const QString &name, const Program &program)
{
    if (!program.isValid() || !name.isValidUtf16()) {
        return std::unexpected(CommandRefusal::InvalidValue);
    }
    return std::make_unique<AddCommand>(store, name, program);
}

CommandOrRefusal makeRemoveCommand(ProgramStore &store, int id)
{
    if (!store.find(id)) {
        return std::unexpected(CommandRefusal::UnknownProgram);
    }
    return std::make_unique<RemoveCommand>(store, id);
}

} // namespace lpd8
