#include "commands.h"

#include "application.h"
#include "programsmodel.h"

CreateProgramCommand::CreateProgramCommand(ProgramsModel* programs, const QString& name, const QByteArray& sysex, QUndoCommand* parent) :
    QUndoCommand(parent),
    m_programs(programs),
    m_name(name),
    m_sysex(sysex),
    m_program_id(-1) {
    Q_CHECK_PTR(m_programs);

    setText("Create program");
}

void CreateProgramCommand::redo() {
    Q_CHECK_PTR(m_programs);

    m_program_id = m_programs->createProgram(m_name, m_sysex);
}

void CreateProgramCommand::undo() {
    Q_CHECK_PTR(m_programs);

    m_programs->deleteProgram(m_program_id);
}

DeleteProgramCommand::DeleteProgramCommand(ProgramsModel* programs, int program_id, QUndoCommand *parent) :
    QUndoCommand(parent),
    m_programs(programs),
    m_program_id(program_id) {
    Q_CHECK_PTR(m_programs);

    setText("Delete program " + m_programs->name(m_program_id));
}

void DeleteProgramCommand::redo() {
    Q_CHECK_PTR(m_programs);

    m_name = m_programs->name(m_program_id);
    m_sysex = m_programs->sysex(m_program_id);
    setObsolete(!m_programs->deleteProgram(m_program_id));
}

void DeleteProgramCommand::undo() {
    Q_CHECK_PTR(m_programs);

    m_program_id = m_programs->createProgram(m_name, m_sysex);
}
