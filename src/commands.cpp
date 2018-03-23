#include "commands.h"

#include "db.h"
#include "programsmodel.h"

#include <QItemSelectionModel>

static const QString undo_stack_object_name = "undo_stack";

QUndoStack* setupUndoStack(QObject* parent) {
    QUndoStack* stack = new QUndoStack(parent);
    stack->setObjectName(undo_stack_object_name);
    return stack;
}

QUndoStack* undoStack() {
    Q_CHECK_PTR(qApp);

    QUndoStack* stack = qApp->findChild<QUndoStack*>(undo_stack_object_name, Qt::FindDirectChildrenOnly);
    if (stack == Q_NULLPTR) {
        stack = setupUndoStack(qApp);
    }
    Q_CHECK_PTR(stack);
    return stack;
}

int selectedProgramId(const QItemSelectionModel *selection_model) {
    Q_CHECK_PTR(selection_model);

    if (!selection_model->hasSelection()) {
        return -1;
    }

    const QAbstractItemModel *model = selection_model->model();
    Q_CHECK_PTR(model);

    const QModelIndex curSelIdx = selection_model->currentIndex();
    const QModelIndex curProjIdx = model->index(curSelIdx.row(), 0);
    return curProjIdx.data(Qt::EditRole).toInt();
}

void setSelectProgramId(QItemSelectionModel *selection_model, int program_id) {
    Q_CHECK_PTR(selection_model);

    const ProgramsModel *model = qobject_cast<ProgramsModel*>(selection_model->model());
    Q_CHECK_PTR(model);

    const QModelIndex programIdx = model->programIndex(program_id);
    Q_ASSERT(programIdx.isValid());

    selection_model->setCurrentIndex(programIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

CreateProgramCommand::CreateProgramCommand(QItemSelectionModel* model, const QString& name, const QByteArray& sysex, QUndoCommand* parent) :
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

    ProgramsModel* model = qobject_cast<ProgramsModel*>(m_model->model());
    Q_CHECK_PTR(model);

    m_program_id = createProgram(m_name, m_sysex, m_program_id);
    if (m_program_id != -1) {
        const int selected_program_id = selectedProgramId(m_model);
        model->select();

        if (selected_program_id != -1) {
            setSelectProgramId(m_model, selected_program_id);
        }
    } else {
        setObsolete(true);
    }
}

void CreateProgramCommand::undo() {
    Q_CHECK_PTR(m_model);

    ProgramsModel* model = qobject_cast<ProgramsModel*>(m_model->model());
    Q_CHECK_PTR(model);

    if (deleteProgram(m_program_id)) {
        const int selected_program_id = selectedProgramId(m_model);
        model->select();
        if (selected_program_id != m_program_id &&
            selected_program_id != -1) {
            setSelectProgramId(m_model, selected_program_id);
        }
    } else {
        setObsolete(true);
    }
}


DeleteProgramCommand::DeleteProgramCommand(ProgramsModel* model, int program_id, QUndoCommand *parent) :
    QUndoCommand(parent),
    m_model(model),
    m_program_id(program_id) {
    Q_CHECK_PTR(m_model);

    setText("Delete program " + programName(m_program_id));
}

void DeleteProgramCommand::redo() {
    Q_CHECK_PTR(m_model);

    m_name = programName(m_program_id);
    m_sysex = programSysex(m_program_id);
    if (deleteProgram(m_program_id)) {
        m_model->select();
    } else {
        setObsolete(true);
    }
}

void DeleteProgramCommand::undo() {
    Q_CHECK_PTR(m_model);

    m_program_id = createProgram(m_name, m_sysex, m_program_id);
    if (m_program_id != -1) {
        m_model->select();
    } else {
        setObsolete(true);
    }
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
