#include "commands.h"

#include "programsmodel.h"

#include <QApplication>
#include <QItemSelectionModel>

#if (QT_VERSION < QT_VERSION_CHECK(5, 9, 0))
void setObsolete(bool) {
}
#endif

static const QString undo_stack_object_name = "undo_stack";

QUndoStack* setupUndoStack(QObject* parent) {
    QUndoStack* stack = new QUndoStack(parent);
    stack->setObjectName(undo_stack_object_name);
    return stack;
}

QUndoStack* undoStack() {
    Q_CHECK_PTR(qApp);

    QUndoStack* stack = qApp->findChild<QUndoStack*>(undo_stack_object_name, Qt::FindDirectChildrenOnly);
    if (stack == nullptr) {
        stack = setupUndoStack(qApp);
    }
    Q_CHECK_PTR(stack);
    return stack;
}


CreateProgramCommand::CreateProgramCommand(ProgramsModel* model, const QString& name, const QByteArray& sysex, QUndoCommand* parent) :
    QUndoCommand(parent),
    m_model(model),
    m_name(name),
    m_sysex(sysex),
    m_program_id(-1)
{
    Q_CHECK_PTR(m_model);

    setText("Create program");
}

void CreateProgramCommand::redo() {
    Q_CHECK_PTR(m_model);

    m_program_id = m_model->createProgram(m_name, m_sysex, m_program_id);
    setObsolete(m_program_id == -1);
}

void CreateProgramCommand::undo() {
    Q_CHECK_PTR(m_model);

    setObsolete(!m_model->deleteProgram(m_program_id));
}


DeleteProgramCommand::DeleteProgramCommand(ProgramsModel* model, int program_id, QUndoCommand *parent) :
    QUndoCommand(parent),
    m_model(model),
    m_program_id(program_id) {
    Q_CHECK_PTR(m_model);

    setText("Delete program");
}

void DeleteProgramCommand::redo() {
    Q_CHECK_PTR(m_model);

    m_name = m_model->programName(m_program_id);
    m_sysex = m_model->programSysex(m_program_id);
    setObsolete(!m_model->deleteProgram(m_program_id));
}

void DeleteProgramCommand::undo() {
    Q_CHECK_PTR(m_model);

    m_program_id = m_model->createProgram(m_name, m_sysex, m_program_id);
    setObsolete(m_program_id == -1);
}


UpdateParameterCommand::UpdateParameterCommand(
    QAbstractItemModel *model,
    int row,
    int column,
    const QVariant &value,
    int role,
    QUndoCommand *parent) :
    QUndoCommand(parent),
    m_model(model),
    m_row(row),
    m_column(column),
    m_value(value),
    m_role(role)
{
    Q_CHECK_PTR(m_model);
}

void UpdateParameterCommand::undo() {
    setDataAndKeepPrevious();
}

void UpdateParameterCommand::redo() {
    setDataAndKeepPrevious();
}

void UpdateParameterCommand::setDataAndKeepPrevious() {
    Q_CHECK_PTR(m_model);

    QVariant newValue = m_value;
    const QModelIndex index = m_model->index(m_row, m_column);
    m_value = index.data(m_role);
    m_model->setData(index, newValue, m_role);
}


UpdateProgramFromSysexCommand::UpdateProgramFromSysexCommand(
    ProgramsModel *model,
    int programId,
    const QByteArray &sysex,
    QUndoCommand *parent) :
    QUndoCommand(parent),
    m_model(model),
    m_program_id(programId),
    m_sysex(sysex) {
    Q_CHECK_PTR(m_model);
}

void UpdateProgramFromSysexCommand::redo() {
    Q_CHECK_PTR(m_model);

    setObsolete(!setSysexAndKeepPrevious());
}

void UpdateProgramFromSysexCommand::undo() {
    Q_CHECK_PTR(m_model);

    setObsolete(!setSysexAndKeepPrevious());
}

bool UpdateProgramFromSysexCommand::setSysexAndKeepPrevious() {
    Q_CHECK_PTR(m_model);

    const QByteArray new_sysex = m_sysex;
    m_sysex = m_model->programSysex(m_program_id);
    return m_model->updateProgramFromSysex(m_program_id, new_sysex);
}
