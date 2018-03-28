#ifndef APPLICATION_H
#define APPLICATION_H

#include "lpd8_types.h"

#include <QObject>

class ProgramsModel;
class MidiIO;

class QAbstractItemModel;
class QSqlTableModel;

class Application : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int activeProgramId READ activeProgramId WRITE setActiveProgramId NOTIFY activeProgramIdChanged)
    Q_PROPERTY(int activeProgramChannel READ activeProgramChannel WRITE setActiveProgramChannel NOTIFY activeProgramChannelChanged)

public:
    explicit Application(QObject *parent = 0);

    int activeProgramId() const;
    int activeProgramChannel() const;

    bool isValidActiveProgram() const;

    MidiIO* midiIO() const;

    QAbstractItemModel* programs() const;
    ProgramsModel* myPrograms() const;
    QAbstractItemModel* pads() const;
    QAbstractItemModel* knobs() const;

public slots:
    void setActiveProgramId(int);
    void setActiveProgramChannel(int);

    void fetchProgram(int) const;
    void sendProgram(int);

    pProgram program(int) const;

signals:
    void activeProgramIdChanged(int);
    void activeProgramChannelChanged(int);
    void connectedChanged(bool);

private slots:
    void refreshModels();

private:
    ProgramsModel* m_my_programs;
    QSqlTableModel* m_programs;
    QSqlTableModel* m_pads;
    QSqlTableModel* m_knobs;

    MidiIO* m_midi_io;
};

#endif // APPLICATION_H
