#include "commands.h"

#include "application.h"
#include "programsmodel.h"

CreateProgramCommand::CreateProgramCommand(ProgramsModel* app, const QString& name, const QByteArray& sysex, QUndoCommand* parent) :
    QUndoCommand(parent),
    m_programs_model(app),
    m_name(name),
    m_sysex(sysex),
    m_program_id(-1)
{
    setText("Create program");
}

void CreateProgramCommand::redo() {
    Q_CHECK_PTR(m_programs_model);

    m_program_id = m_programs_model->createProgram(m_name, m_sysex);
}

void CreateProgramCommand::undo() {
    Q_CHECK_PTR(m_programs_model);

    m_programs_model->deleteProgram(m_program_id);
//    m_app->deleteProgram(m_program_id);
}

DeleteProgramCommand::DeleteProgramCommand(Application* app, int program_id, QUndoCommand *parent) :
    QUndoCommand(parent),
    m_app(app),
    m_program_id(program_id) {
    setText("Delete program");
}

void DeleteProgramCommand::redo() {
    Q_CHECK_PTR(m_app);
    Q_ASSERT(m_program_id > -1);

    m_program = m_app->program(m_program_id);
    m_app->deleteProgram(m_program_id);
}

void DeleteProgramCommand::undo() {
    Q_CHECK_PTR(m_app);
    Q_ASSERT(!m_program.isNull());

    m_program_id = m_app->newProgram("Not implemented yet", QByteArray());
    m_program->id = m_program_id;
    m_app->onProgramFetched(m_program);
}
