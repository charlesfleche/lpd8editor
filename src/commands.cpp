#include "commands.h"

#include "programsmodel.h"

#include <QCoreApplication>
#include <QItemSelectionModel>
#include <utility>

namespace {

const QString undo_stack_object_name = "undo_stack";

QUndoStack *setupUndoStack(QObject *parent)
{
    auto *stack = new QUndoStack(parent);
    stack->setObjectName(undo_stack_object_name);
    return stack;
}

} // namespace

QUndoStack *undoStack()
{
    QCoreApplication *app = QCoreApplication::instance();
    Q_ASSERT(app);

    auto *stack = app->findChild<QUndoStack *>(undo_stack_object_name, Qt::FindDirectChildrenOnly);
    if (stack == nullptr) {
        stack = setupUndoStack(app);
    }
    return stack;
}

CreateProgramCommand::CreateProgramCommand(ProgramsModel *model,
                                           QString name,
                                           QByteArray sysex,
                                           QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_name(std::move(name))
    , m_sysex(std::move(sysex))
{
    Q_ASSERT(m_model);

    setText("Create program");
}

void CreateProgramCommand::redo()
{
    m_program_id = m_model->createProgram(m_name, m_sysex, m_program_id);
    setObsolete(m_program_id == -1);
}

void CreateProgramCommand::undo()
{
    setObsolete(!m_model->deleteProgram(m_program_id));
}

DeleteProgramCommand::DeleteProgramCommand(ProgramsModel *model,
                                           int programId,
                                           QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_program_id(programId)
{
    Q_ASSERT(m_model);

    setText("Delete program");
}

void DeleteProgramCommand::redo()
{
    m_name = m_model->programName(m_program_id);
    m_sysex = m_model->programSysex(m_program_id);
    setObsolete(!m_model->deleteProgram(m_program_id));
}

void DeleteProgramCommand::undo()
{
    m_program_id = m_model->createProgram(m_name, m_sysex, m_program_id);
    setObsolete(m_program_id == -1);
}

UpdateParameterCommand::UpdateParameterCommand(ProgramsModel *model,
                                               ParameterLocation location,
                                               QVariant value,
                                               QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_location(location)
    , m_value(std::move(value))
{
    Q_ASSERT(m_model);
}

void UpdateParameterCommand::undo()
{
    setDataAndKeepPrevious();
}

void UpdateParameterCommand::redo()
{
    setDataAndKeepPrevious();
}

void UpdateParameterCommand::setDataAndKeepPrevious()
{
    const QModelIndex index = m_model->parameterIndex(m_location);
    if (!index.isValid()) {
        // The value no longer exists: nothing to undo or redo.
        setObsolete(true);
        return;
    }
    const QVariant newValue = m_value;
    m_value = index.data(Qt::EditRole);
    if (!m_model->writeParameter(index, newValue)) {
        m_value = newValue;
        setObsolete(true);
    }
}

UpdateProgramFromSysexCommand::UpdateProgramFromSysexCommand(ProgramsModel *model,
                                                             int programId,
                                                             QByteArray sysex,
                                                             QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_program_id(programId)
    , m_sysex(std::move(sysex))
{
    Q_ASSERT(m_model);
}

void UpdateProgramFromSysexCommand::redo()
{
    setObsolete(!setSysexAndKeepPrevious());
}

void UpdateProgramFromSysexCommand::undo()
{
    setObsolete(!setSysexAndKeepPrevious());
}

bool UpdateProgramFromSysexCommand::setSysexAndKeepPrevious()
{
    const QByteArray new_sysex = m_sysex;
    m_sysex = m_model->programSysex(m_program_id);
    return m_model->updateProgramFromSysex(m_program_id, new_sysex);
}
