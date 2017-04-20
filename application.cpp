#include "application.h"

#include "db.h"
#include "midiio.h"
#include "utils.h"

#include <QSettings>
#include <QSqlTableModel>
#include <QtDebug>

#include <exception>

static const QString SETTINGS_KEY_ACTIVE_PRESET_ID = "active/preset_id";
static const QString SETTINGS_KEY_ACTIVE_PROGRAM_ID = "active/program_id";

Application::Application(QObject *parent):
    QObject(parent),
    m_presets(Q_NULLPTR),
    m_preset_pads(Q_NULLPTR),
    m_preset_knobs(Q_NULLPTR),
    m_program_pads(Q_NULLPTR),
    m_program_knobs(Q_NULLPTR),
    m_midi_io(Q_NULLPTR)
{
    if (!initFilesystem()) {
        throw std::runtime_error("Failed filesystem initialization");
    }
    if (initDb(defaultDbPath()).isValid()) {
        throw std::runtime_error("Failed database initialization");
    }

    m_presets = new QSqlTableModel(this);
    m_presets->setTable("presets");
    m_presets->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_presets->select();

    m_preset_pads = new QSqlTableModel(this);
    m_preset_pads->setTable("pads");
    m_preset_pads->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_preset_pads->select();

    m_preset_knobs = new QSqlTableModel(this);
    m_preset_knobs->setTable("knobs");
    m_preset_knobs->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_preset_knobs->select();

    m_program_pads = new QSqlTableModel(this);
    m_program_pads->setTable("pads");
    m_program_pads->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_program_pads->select();

    m_program_knobs = new QSqlTableModel(this);
    m_program_knobs->setTable("knobs");
    m_program_knobs->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_program_knobs->select();

    connect(this,
            &Application::activeProgramIdChanged,
            &Application::refreshModels);
    connect(this,
            &Application::activePresetIdChanged,
            &Application::refreshModels);

    refreshModels();

    m_midi_io = new MidiIO(this);
    connect(m_midi_io,
            &MidiIO::programReceived,
            this,
            &Application::onProgramFetched);
}

QAbstractItemModel* Application::presets() const {
    return m_presets;
}

QAbstractItemModel* Application::pads() const {
    return m_program_pads;
}

QAbstractItemModel* Application::knobs() const {
    return m_program_knobs;
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
    const int presetId = QSettings().value(SETTINGS_KEY_ACTIVE_PRESET_ID, 0).toInt();
    return isValidPresetId(presetId) ? presetId : 0;
}

void Application::setActivePresetId(int presetId) {
    if (activePresetId() == presetId) {
        return;
    }
    QSettings().setValue(SETTINGS_KEY_ACTIVE_PRESET_ID, presetId);
    emit activePresetIdChanged(presetId);
}

int Application::activeProgramId() const {
    const int programId = QSettings().value(SETTINGS_KEY_ACTIVE_PROGRAM_ID, 0).toInt();
    return isValidProgramId(programId) ? programId : 1;
}

void Application::setActiveProgramId(int programId) {
    if (activeProgramId() == programId) {
        return;
    }
    QSettings().setValue(SETTINGS_KEY_ACTIVE_PROGRAM_ID, programId);
    emit activeProgramIdChanged(programId);;
}

void Application::refreshModels() {
    Q_CHECK_PTR(m_program_pads);
    Q_CHECK_PTR(m_program_knobs);

    qDebug() << "Application::refreshModel" << activePresetId() << activeProgramId();

    QString filter = QString("presetId=%1").arg(activePresetId());
    m_preset_pads->setFilter(filter);
    m_preset_knobs->setFilter(filter);

    filter = QString("presetId=%1 AND programId=%2")
             .arg(activePresetId())
             .arg(activeProgramId());
    m_program_pads->setFilter(filter);
    m_program_knobs->setFilter(filter);
}

bool Application::connected() const {
    return false;
}

void Application::setConnected(bool) {
}

void Application::fetchPrograms() const {
    Q_CHECK_PTR(m_midi_io);

    // XXX Should be based on a model, not static program numbers
//    for (int i = 1 ; i <= 4 ; ++i) {
//        m_midi_io->getProgram(i);
//    }
    m_midi_io->getPrograms();
}

void Application::onProgramFetched(pProgram p) {
    Q_UNUSED(p)

    const int programId = static_cast<int>(p->id);
    const int row_offset = (programId - 1)*8;

    for (int i = 0 ; i < 8 ; ++i) {
        const int row = row_offset + i;
        QSqlRecord r;
        r.append(QSqlField("note", QVariant::Int));
        r.setValue("note", p->pads[i].note);

        r.append(QSqlField("pc", QVariant::Int));
        r.setValue("pc", p->pads[i].pc);

        r.append(QSqlField("cc", QVariant::Int));
        r.setValue("cc", p->pads[i].cc);

        r.append(QSqlField("momentary", QVariant::Int));
        r.setValue("momentary", p->pads[i].momentary);
//        qDebug() << "Set pad" << programId << i;
        if (!m_preset_pads->setRecord(row, r)) {
            qDebug() << "Cannot set pad" << programId << i << r;
        }

        r.clear();

        r.append(QSqlField("cc", QVariant::Int));
        r.setValue("cc", p->pads[i].cc);
//        qDebug() << "Set knob" << programId << i;
        if (!m_preset_knobs->setRecord(row, r)) {
            qDebug() << "Cannot set knob" << programId << i << r;
        }
    }

    refreshModels();
}
