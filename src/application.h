#ifndef APPLICATION_H
#define APPLICATION_H

#include "lpd8_types.h"

#include <QObject>

class MidiIO;

class QAbstractItemModel;
class QSqlTableModel;

class Application : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int activeProgramId READ activeProgramId WRITE setActiveProgramId NOTIFY activeProgramIdChanged)

public:
    explicit Application(QObject *parent = 0);

    int activeProgramId() const;

    QAbstractItemModel* programs() const;
    QAbstractItemModel* pads() const;
    QAbstractItemModel* knobs() const;

public slots:
    void setActiveProgramId(int);

    void newProgram();
    void deleteProgram(int);

    void fetchProgram(int) const;
    void sendProgram(int);

signals:
    void activeProgramIdChanged(int);
    void connectedChanged(bool);

private slots:
    void refreshModels();

    void onProgramFetched(pProgram);

private:
    QSqlTableModel* m_programs;
    QSqlTableModel* m_pads;
    QSqlTableModel* m_knobs;

    MidiIO* m_midi_io;
};

#endif // APPLICATION_H
