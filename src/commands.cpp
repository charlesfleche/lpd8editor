#include "commands.h"

#include "programsmodel.h"

#include <QApplication>
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

int selectedProgramId(QItemSelectionModel* model) {
    Q_CHECK_PTR(model);

    if (!model->hasSelection()) {
        return -1;
    }

    const QAbstractItemModel *m = model->model();
    Q_CHECK_PTR(m);

    const QModelIndex curSelIdx = model->currentIndex();
    const QModelIndex curProjIdx = m->index(curSelIdx.row(), 0);
    return curProjIdx.data(Qt::EditRole).toInt();
}


class ProgramIdSelectionRestorer {
public:
    ProgramIdSelectionRestorer(QItemSelectionModel* model);

    void store();
    void restore();

private:
    int selectedProgramId() const;
    void selectProgramId(int programId);

    QItemSelectionModel *m_model;
    int m_program_id;
};

ProgramIdSelectionRestorer::ProgramIdSelectionRestorer(QItemSelectionModel *model) :
    m_model(model)
{
    Q_CHECK_PTR(m_model);

    store();
}

void ProgramIdSelectionRestorer::store() {
    Q_CHECK_PTR(m_model);

    m_program_id = selectedProgramId();
}

void ProgramIdSelectionRestorer::restore() {
    Q_CHECK_PTR(m_model);

    const ProgramsModel *m = qobject_cast<ProgramsModel*>(m_model->model());
    Q_CHECK_PTR(m);

    if (m->programIndex(m_program_id).isValid()) {
        selectProgramId(m_program_id);
    }
}

int ProgramIdSelectionRestorer::selectedProgramId() const {
    Q_CHECK_PTR(m_model);

    return ::selectedProgramId(m_model);
}

void ProgramIdSelectionRestorer::selectProgramId(int program_id) {
    Q_CHECK_PTR(m_model);

    const ProgramsModel *m = qobject_cast<ProgramsModel*>(m_model->model());
    Q_CHECK_PTR(m);

    const QModelIndex programIdx = m->programIndex(program_id);
    Q_ASSERT(programIdx.isValid());

    m_model->setCurrentIndex(programIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
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

    ProgramIdSelectionRestorer sel(m_model);
    m_program_id = model->createProgram(m_name, m_sysex, m_program_id);
    if (m_program_id != -1) {
        sel.restore();
    } else {
        setObsolete(true);
    }
}

void CreateProgramCommand::undo() {
    Q_CHECK_PTR(m_model);

    ProgramsModel* model = qobject_cast<ProgramsModel*>(m_model->model());
    Q_CHECK_PTR(model);

    ProgramIdSelectionRestorer sel(m_model);
    if (model->deleteProgram(m_program_id)) {
        sel.restore();
    } else {
        setObsolete(true);
    }
}


DeleteProgramCommand::DeleteProgramCommand(QItemSelectionModel* model, int program_id, QUndoCommand *parent) :
    QUndoCommand(parent),
    m_model(model),
    m_program_id(program_id) {
    Q_CHECK_PTR(m_model);

    setText("Delete program");
}

void DeleteProgramCommand::redo() {
    Q_CHECK_PTR(m_model);

    ProgramsModel* model = qobject_cast<ProgramsModel*>(m_model->model());
    Q_CHECK_PTR(model);

    m_name = model->programName(m_program_id);
    m_sysex = model->programSysex(m_program_id);
    ProgramIdSelectionRestorer sel(m_model);
    if (model->deleteProgram(m_program_id)) {
        sel.restore();
    } else {
        setObsolete(true);
    }
}

void DeleteProgramCommand::undo() {
    Q_CHECK_PTR(m_model);

    ProgramsModel* model = qobject_cast<ProgramsModel*>(m_model->model());
    Q_CHECK_PTR(model);

    m_program_id = model->createProgram(m_name, m_sysex, m_program_id);
    ProgramIdSelectionRestorer sel(m_model);
    if (m_program_id != -1) {
        sel.restore();
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
