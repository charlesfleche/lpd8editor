#include "commands.h"

#include "application.h"

CreateProgramCommand::CreateProgramCommand(Application* app, const QString& name, QUndoCommand* parent) :
    QUndoCommand(parent),
    m_app(app),
    m_name(name),
    m_program_id(-1)
{
    setText("Create program");
}

void CreateProgramCommand::redo() {
    Q_CHECK_PTR(m_app);

    m_program_id = m_app->newProgram(m_name);
}

void CreateProgramCommand::undo() {
    Q_CHECK_PTR(m_app);
    Q_ASSERT(m_program_id > -1);

    m_app->deleteProgram(m_program_id);
}
