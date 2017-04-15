#include "application.h"

#include "db.h"
#include "utils.h"

#include <QSettings>
#include <QSqlTableModel>

#include <exception>

static const QString SETTINGS_KEY_ACTIVE_PRESET_ID = "active/preset_id";
static const QString SETTINGS_KEY_ACTIVE_PROGRAM_ID = "active/program_id";

Application::Application(QObject *parent):
    QObject(parent),
    m_presets(Q_NULLPTR),
    m_pads(Q_NULLPTR),
    m_knobs(Q_NULLPTR)
{
    if (!initFilesystem()) {
        throw std::exception();
    }
    if (initDb(defaultDbPath()).isValid()) {
        throw std::exception();
    }

    m_presets = new QSqlTableModel(this);
    m_presets->setTable("presets");
    m_presets->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_presets->select();

    m_pads = new QSqlTableModel(this);
    m_pads->setTable("pads");
    m_pads->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_pads->select();

    m_knobs = new QSqlTableModel(this);
    m_knobs->setTable("knobs");
    m_knobs->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_knobs->select();

    connect(this,
            &Application::activeProgramIdChanged,
            &Application::refreshModels);
    connect(this,
            &Application::activePresetIdChanged,
            &Application::refreshModels);

    refreshModels();
}

QAbstractItemModel* Application::presets() const {
    return m_presets;
}

QAbstractItemModel* Application::pads() const {
    return m_pads;
}

QAbstractItemModel* Application::knobs() const {
    return m_knobs;
}

void Application::newPreset() {
    addPreset("New preset");
    m_presets->select();
}

void Application::deletePreset(int presetId) {
    ::deletePreset(presetId);
    m_presets->select();
}

int Application::activePresetId() const {
    const int ret = QSettings().value(SETTINGS_KEY_ACTIVE_PRESET_ID).toInt();
    return ret;
}

void Application::setActivePresetId(int presetId) {
    if (activePresetId() == presetId) {
        return;
    }
    QSettings().setValue(SETTINGS_KEY_ACTIVE_PRESET_ID, presetId);
    emit activePresetIdChanged(presetId);
}

int Application::activeProgramId() const {
    const int ret = QSettings().value(SETTINGS_KEY_ACTIVE_PROGRAM_ID).toInt();
    return ret;
}

void Application::setActiveProgramId(int programId) {
    if (activeProgramId() == programId) {
        return;
    }
    QSettings().setValue(SETTINGS_KEY_ACTIVE_PROGRAM_ID, programId);
    emit activeProgramIdChanged(programId);;
}

void Application::refreshModels() {
    Q_CHECK_PTR(m_pads);
    Q_CHECK_PTR(m_knobs);
    qDebug() << "Application::refreshModel" << activePresetId() << activeProgramId();
    const QString filter = QString("presetId=%1 AND programId=%2")
                           .arg(activePresetId())
                           .arg(activeProgramId());
    m_pads->setFilter(filter);
    m_knobs->setFilter(filter);
}
