#ifndef APPLICATION_H
#define APPLICATION_H

#include "lpd8_types.h"

#include <QObject>

class PadProxyModel;
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

    QAbstractItemModel* programs() const;
    QAbstractItemModel* pads() const;
    QAbstractItemModel* knobs() const;

public slots:
    void setActiveProgramId(int);
    void setActiveProgramChannel(int);

    int newProgram(const QString&);
    void deleteProgram(int);

    void fetchProgram(int) const;
    void sendProgram(int);

    void exportActiveProgram(const QString&) const;
    void importProgram(const QString&);

signals:
    void activeProgramIdChanged(int);
    void activeProgramChannelChanged(int);
    void connectedChanged(bool);

private slots:
    void refreshModels();

    void onProgramFetched(pProgram);

private:
    pProgram program(int) const;

    QSqlTableModel* m_programs;
    QSqlTableModel* m_pads;
    PadProxyModel* m_pads_proxy;
    QSqlTableModel* m_knobs;

    MidiIO* m_midi_io;
};

#endif // APPLICATION_H
