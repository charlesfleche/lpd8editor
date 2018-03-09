#ifndef COMMANDS_H
#define COMMANDS_H

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

#endif // COMMANDS_H
