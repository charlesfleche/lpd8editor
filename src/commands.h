#ifndef COMMANDS_H
#define COMMANDS_H

#include "lpd8_types.h"

#include <QUndoCommand>

class Application;

class CreateProgramCommand : public QUndoCommand {
public:
    CreateProgramCommand(Application*, const QString&, QUndoCommand* parent = Q_NULLPTR);

    void undo() Q_DECL_OVERRIDE;
    void redo() Q_DECL_OVERRIDE;

private:
    Application* m_app;
    const QString m_name;
    int m_program_id;
};

class DeleteProgramCommand : public QUndoCommand {
public:
    DeleteProgramCommand(Application*, int, QUndoCommand* parent = Q_NULLPTR);

    void undo() Q_DECL_OVERRIDE;
    void redo() Q_DECL_OVERRIDE;

private:
    Application* m_app;
    int m_program_id;
    pProgram m_program;
};

#endif // COMMANDS_H
