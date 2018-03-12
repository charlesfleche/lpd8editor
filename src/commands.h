#ifndef COMMANDS_H
#define COMMANDS_H

#include "lpd8_types.h"

#include <QUndoCommand>

class Application;
class ProgramsModel;

class CreateProgramCommand : public QUndoCommand {
public:
    CreateProgramCommand(ProgramsModel*, const QString&, const QByteArray&, QUndoCommand* parent = Q_NULLPTR);

    void undo() Q_DECL_OVERRIDE;
    void redo() Q_DECL_OVERRIDE;

private:
    ProgramsModel* m_programs;
    const QString m_name;
    const QByteArray m_sysex;
    int m_program_id;
};

class DeleteProgramCommand : public QUndoCommand {
public:
    DeleteProgramCommand(ProgramsModel*, int, QUndoCommand* parent = Q_NULLPTR);

    void undo() Q_DECL_OVERRIDE;
    void redo() Q_DECL_OVERRIDE;

private:
    ProgramsModel* m_programs;
    int m_program_id;
    QString m_name;
    QByteArray m_sysex;
};

#endif // COMMANDS_H
