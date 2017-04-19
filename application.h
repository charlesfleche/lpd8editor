#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>

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

signals:
    void activePresetIdChanged(int);
    void activeProgramIdChanged(int);
    void connectedChanged(bool);

private slots:
    void refreshModels();

private:
    QSqlTableModel* m_presets;
    QSqlTableModel* m_pads;
    QSqlTableModel* m_knobs;
};

#endif // APPLICATION_H
