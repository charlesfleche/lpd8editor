#ifndef COMMANDS_H
#define COMMANDS_H

#include "parameterlocation.h"

#include <QUndoCommand>
#include <QVariant>

class ProgramsModel;
class QItemSelectionModel;
class QModelIndex;
class QUndoStack;

QUndoStack *undoStack();

int selectedProgramId(QItemSelectionModel *model);

class CreateProgramCommand : public QUndoCommand
{
public:
    CreateProgramCommand(ProgramsModel *model,
                         QString name,
                         QByteArray sysex,
                         QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    ProgramsModel *m_model;
    const QString m_name;
    const QByteArray m_sysex;
    int m_program_id{-1};
};

class DeleteProgramCommand : public QUndoCommand
{
public:
    DeleteProgramCommand(ProgramsModel *model, int programId, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    ProgramsModel *m_model;
    int m_program_id;
    QString m_name;
    QByteArray m_sysex;
};

class UpdateParameterCommand : public QUndoCommand
{
public:
    UpdateParameterCommand(ProgramsModel *model,
                           ParameterLocation location,
                           QVariant value,
                           QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    void setDataAndKeepPrevious();

    // The model lives as long as the window, and the undo stack with it.
    // What the command must *not* hold is anything the model rebuilds, such
    // as its internal sub-models: m_location is resolved again at each
    // undo and redo.
    ProgramsModel *m_model;
    const ParameterLocation m_location;
    QVariant m_value;
};

class UpdateProgramFromSysexCommand : public QUndoCommand
{
public:
    UpdateProgramFromSysexCommand(ProgramsModel *model,
                                  int programId,
                                  QByteArray sysex,
                                  QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    bool setSysexAndKeepPrevious();

    ProgramsModel *m_model;
    const int m_program_id;
    QByteArray m_sysex;
};

#endif // COMMANDS_H
