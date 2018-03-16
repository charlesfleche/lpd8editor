#include "commands.h"

#include "db.h"

CreateProgramCommand::CreateProgramCommand(const QString& name, const QByteArray& sysex, QUndoCommand* parent) :
    QUndoCommand(parent),
    m_name(name),
    m_sysex(sysex),
    m_program_id(-1) {
    setText("Create program");
}

void CreateProgramCommand::redo() {
    m_program_id = createProgram(m_name, m_sysex);
}

void CreateProgramCommand::undo() {
    deleteProgram(m_program_id);
}

DeleteProgramCommand::DeleteProgramCommand(int program_id, QUndoCommand *parent) :
    QUndoCommand(parent),
    m_program_id(program_id) {
    setText("Delete program " + programName(m_program_id));
}

void DeleteProgramCommand::redo() {
    m_name = programName(m_program_id);
    m_sysex = programSysex(m_program_id);
    setObsolete(!deleteProgram(m_program_id));
}

void DeleteProgramCommand::undo() {
    m_program_id = createProgram(m_name, m_sysex);
}
