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
    Q_PROPERTY(int activePresetId READ activePresetId WRITE setActivePresetId NOTIFY activePresetIdChanged)
    Q_PROPERTY(int activeProgramId READ activeProgramId WRITE setActiveProgramId NOTIFY activeProgramIdChanged)
    Q_PROPERTY(QAbstractItemModel* presets READ presets CONSTANT)
    Q_PROPERTY(bool connected READ connected WRITE setConnected NOTIFY connectedChanged)

public:
    explicit Application(QObject *parent = 0);

    int activePresetId() const;
    int activeProgramId() const;

    QAbstractItemModel* presets() const;
    QAbstractItemModel* pads() const;
    QAbstractItemModel* knobs() const;

    bool connected() const;

public slots:
    void setActivePresetId(int);
    void setActiveProgramId(int);

    void newPreset();
    void deletePreset(int presetId);

    void setConnected(bool);

    void fetchPrograms() const;
    void sendPrograms();

signals:
    void activePresetIdChanged(int);
    void activeProgramIdChanged(int);
    void connectedChanged(bool);

private slots:
    void refreshModels();

    void onProgramFetched(pProgram);

private:
    QSqlTableModel* m_presets;
    QSqlTableModel* m_preset_pads;
    QSqlTableModel* m_preset_knobs;
    QSqlTableModel* m_program_pads;
    QSqlTableModel* m_program_knobs;

    MidiIO* m_midi_io;
};

#endif // APPLICATION_H
