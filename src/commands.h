#ifndef COMMANDS_H
#define COMMANDS_H

#include <QUndoCommand>
#include <QVariant>

class ProgramsModel;

class QAbstractItemModel;
class QItemSelectionModel;
class QModelIndex;
class QUndoStack;

QUndoStack* undoStack();

int selectedProgramId(QItemSelectionModel* model);

class CreateProgramCommand : public QUndoCommand {
public:
    CreateProgramCommand(QItemSelectionModel* model, const QString&, const QByteArray&, QUndoCommand* parent = Q_NULLPTR);

    void undo() Q_DECL_OVERRIDE;
    void redo() Q_DECL_OVERRIDE;

private:
    QItemSelectionModel *m_model;
    const QString m_name;
    const QByteArray m_sysex;
    int m_program_id;
};

class DeleteProgramCommand : public QUndoCommand {
public:
    DeleteProgramCommand(QItemSelectionModel* model, int programId, QUndoCommand* parent = Q_NULLPTR);

    void undo() Q_DECL_OVERRIDE;
    void redo() Q_DECL_OVERRIDE;

private:
    QItemSelectionModel *m_model;
    int m_program_id;
    QString m_name;
    QByteArray m_sysex;
};

class UpdateParameterCommand : public QUndoCommand {
public:
    UpdateParameterCommand(
        QAbstractItemModel* model,
        int row,
        int column,
        const QVariant& value,
        int role,
        QUndoCommand *parent = Q_NULLPTR
    );

    void undo() Q_DECL_OVERRIDE;
    void redo() Q_DECL_OVERRIDE;

private:
    void setDataAndKeepPrevious();

    QAbstractItemModel* m_model;
    const int m_row;
    const int m_column;
    QVariant m_value;
    const int m_role;
};

class UpdateProgramFromSysexCommand : public QUndoCommand {
public:
    UpdateProgramFromSysexCommand(
        ProgramsModel* model,
        int programId,
        const QByteArray& sysex,
        QUndoCommand *parent = Q_NULLPTR);

    void undo() Q_DECL_OVERRIDE;
    void redo() Q_DECL_OVERRIDE;

private:
    bool setSysexAndKeepPrevious();

    ProgramsModel *m_model;
    const int m_program_id;
    QByteArray m_sysex;
};

#endif // COMMANDS_H
