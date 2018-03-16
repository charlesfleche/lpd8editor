#ifndef COMMANDS_H
#define COMMANDS_H

#include <QUndoCommand>

class CreateProgramCommand : public QUndoCommand {
public:
    CreateProgramCommand(const QString&, const QByteArray&, QUndoCommand* parent = Q_NULLPTR);

    void undo() Q_DECL_OVERRIDE;
    void redo() Q_DECL_OVERRIDE;

private:
    const QString m_name;
    const QByteArray m_sysex;
    int m_program_id;
};

class DeleteProgramCommand : public QUndoCommand {
public:
    DeleteProgramCommand(int, QUndoCommand* parent = Q_NULLPTR);

    void undo() Q_DECL_OVERRIDE;
    void redo() Q_DECL_OVERRIDE;

private:
    int m_program_id;
    QString m_name;
    QByteArray m_sysex;
};

#endif // COMMANDS_H
