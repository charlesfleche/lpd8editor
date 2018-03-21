#ifndef COMMANDS_H
#define COMMANDS_H

#include <QUndoCommand>
#include <QVariant>

class ProgramsModel;

class QAbstractItemModel;
class QModelIndex;
class QUndoStack;

QUndoStack* undoStack();

class CreateProgramCommand : public QUndoCommand {
public:
    CreateProgramCommand(ProgramsModel* model, const QString&, const QByteArray&, QUndoCommand* parent = Q_NULLPTR);

    void undo() Q_DECL_OVERRIDE;
    void redo() Q_DECL_OVERRIDE;

private:
    ProgramsModel *m_model;
    const QString m_name;
    const QByteArray m_sysex;
    int m_program_id;
};

class DeleteProgramCommand : public QUndoCommand {
public:
    DeleteProgramCommand(ProgramsModel* model, int programId, QUndoCommand* parent = Q_NULLPTR);

    void undo() Q_DECL_OVERRIDE;
    void redo() Q_DECL_OVERRIDE;

private:
    ProgramsModel *m_model;
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

#endif // COMMANDS_H
