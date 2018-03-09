#include "application.h"

#include "db.h"
#include "midiio.h"
#include "padproxymodel.h"
#include "programsmodel.h"
#include "utils.h"

#include <QFileInfo>
#include <QSettings>
#include <QSqlTableModel>
#include <QtDebug>

#include <exception>

static const QString SETTINGS_KEY_ACTIVE_PROGRAM_ID = "active/program_id";

Application::Application(QObject *parent):
    QObject(parent),
    m_my_programs(Q_NULLPTR),
    m_programs(Q_NULLPTR),
    m_pads(Q_NULLPTR),
    m_pads_proxy(Q_NULLPTR),
    m_knobs(Q_NULLPTR),
    m_midi_io(Q_NULLPTR)
{
    if (!initFilesystem()) {
        throw std::runtime_error("Failed filesystem initialization");
    }
    if (initDb(defaultDbPath()).isValid()) {
        throw std::runtime_error("Failed database initialization");
    }

    m_my_programs = new ProgramsModel(this);

    m_programs = new QSqlTableModel(this);
    m_programs->setTable("programs");
    m_programs->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_programs->select();

    m_pads = new QSqlTableModel(this);
    m_pads->setTable("pads");
    m_pads->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_pads->select();

    m_pads_proxy = new PadProxyModel(this);
    m_pads_proxy->setSourceModel(m_pads);

    m_knobs = new QSqlTableModel(this);
    m_knobs->setTable("knobs");
    m_knobs->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_knobs->select();

    connect(this,
            &Application::activeProgramIdChanged,
            &Application::refreshModels);

    refreshModels();

    m_midi_io = new MidiIO(this);
    connect(m_midi_io,
            &MidiIO::programReceived,
            this,
            &Application::onProgramFetched);
}

QAbstractItemModel* Application::programs() const {
    return m_my_programs;
}

QAbstractItemModel* Application::pads() const {
    return m_pads_proxy;
}

QAbstractItemModel* Application::knobs() const {
    return m_knobs;
}

int Application::newProgram(const QString& name) {
    int programId;
    if (!addProgram(name, programId)) {
        throw std::runtime_error("Failed to add program");
    }
    m_programs->select();
    setActiveProgramId(programId);
    return programId;
}

void Application::deleteProgram(int programId) {
    const int nextId = nextProgramId(programId);
    ::deleteProgram(programId);
    setActiveProgramId(nextId);
    m_programs->select();
}

int Application::activeProgramId() const {
    const int programId = QSettings().value(SETTINGS_KEY_ACTIVE_PROGRAM_ID, -1).toInt();
    return isValidProgramId(programId) ? programId : -1;
}

MidiIO* Application::midiIO() const {
    Q_CHECK_PTR(m_midi_io);

    return m_midi_io;
}

int Application::activeProgramChannel() const
{
    Q_CHECK_PTR(programs());

    for (int row = 0 ; row < programs()->rowCount() ; ++row) {
        const int programId(getProgramId(programs(), row));
        if (programId == activeProgramId()) {
            return programs()->data(programs()->index(row, 2)).toInt();
        }
    }

    return -1;
}

void Application::setActiveProgramId(int programId) {
    if (QSettings().value(SETTINGS_KEY_ACTIVE_PROGRAM_ID).toInt() == programId) {
        return;
    }
    QSettings().setValue(SETTINGS_KEY_ACTIVE_PROGRAM_ID, programId);
    emit activeProgramIdChanged(programId);
    emit activeProgramChannelChanged(activeProgramChannel());
}

void Application::setActiveProgramChannel(int channel)
{
    Q_CHECK_PTR(programs());

    for (int row = 0 ; row < programs()->rowCount() ; ++row) {
        const int programId(getProgramId(programs(), row));
        if (programId == activeProgramId()) {
            const QModelIndex idx(programs()->index(row, 2));
            const int curChannel(programs()->data(idx).toInt());
            if (channel != curChannel) {
                programs()->setData(idx, channel);
                emit activeProgramChannelChanged(channel);
            }
        }
    }
}

void Application::refreshModels() {
    Q_CHECK_PTR(m_pads);
    Q_CHECK_PTR(m_knobs);

    const QString filter = QString("programId=%1").arg(activeProgramId());
    m_pads->setFilter(filter);
    m_knobs->setFilter(filter);
}

void Application::fetchProgram(int programId) const {
    Q_CHECK_PTR(m_midi_io);

    m_midi_io->getProgram(programId);
}

char getChar(const QSqlRecord& r, const QString& name) {
    return r.value(name).toChar().toLatin1();
}

void Application::sendProgram(int programId) {
    m_midi_io->sendProgram(program(programId));
}

void Application::exportActiveProgram(const QString & path) const {
    writeProgramFile(program(activeProgramId()), path);
}

void Application::importProgram(const QString & path) {
    pProgram p = readProgramFile(path);
    if (!p) {
        return;
    }

    const QString fileName(QFileInfo(path).baseName());
    const int programId = newProgram(fileName);
    setActiveProgramId(programId);
    onProgramFetched(p);
}

void Application::onProgramFetched(pProgram p) {
    Q_CHECK_PTR(p);

    const int programId = static_cast<int>(p->id);

    for (int i = 0 ; i < 8 ; ++i) {
        QSqlRecord r;
        r.append(QSqlField("note", QVariant::Int));
        r.setValue("note", p->pads[i].note);

        r.append(QSqlField("pc", QVariant::Int));
        r.setValue("pc", p->pads[i].pc);

        r.append(QSqlField("cc", QVariant::Int));
        r.setValue("cc", p->pads[i].cc);

        r.append(QSqlField("toggle", QVariant::Int));
        r.setValue("toggle", p->pads[i].toggle);
        if (!m_pads->setRecord(i, r)) {
            qDebug() << "Cannot set pad" << programId << i << r;
        }

        r.clear();

        r.append(QSqlField("cc", QVariant::Int));
        r.setValue("cc", p->knobs[i].cc);

        r.append(QSqlField("low", QVariant::Int));
        r.setValue("low", p->knobs[i].low);

        r.append(QSqlField("high", QVariant::Int));
        r.setValue("high", p->knobs[i].high);

        if (!m_knobs->setRecord(i, r)) {
            qDebug() << "Cannot set knob" << programId << i << r;
        }
    }

    refreshModels();
}

pProgram Application::program(int programId) const {
    Q_CHECK_PTR(m_midi_io);

    pProgram p(new Program());
    p->id = static_cast<char>(programId);
    p->channel = 1;

    for (int i = 0 ; i < m_pads->rowCount() ; ++i ) {
        QSqlRecord r = m_pads->record(i);

        Q_ASSERT(r.contains("programId"));
        Q_ASSERT(r.contains("note"));
        Q_ASSERT(r.contains("pc"));
        Q_ASSERT(r.contains("cc"));
        Q_ASSERT(r.contains("toggle"));

        const int padIndex = r.value("controlId").toInt()-1;
        Pad& pad = p->pads[padIndex];
        pad.note = getChar(r, "note");
        pad.pc = getChar(r, "pc");
        pad.cc = getChar(r, "cc");
        pad.toggle = getChar(r, "toggle");
    }

    for (int i = 0 ; i < m_knobs->rowCount() ; ++i ) {
        QSqlRecord r = m_knobs->record(i);

        Q_ASSERT(r.contains("programId"));
        Q_ASSERT(r.contains("cc"));
        Q_ASSERT(r.contains("low"));
        Q_ASSERT(r.contains("high"));

        const int knobIndex = r.value("controlId").toInt()-1;
        Knob& knob = p->knobs[knobIndex];
        knob.cc = getChar(r, "cc");
        knob.low = getChar(r, "low");
        knob.high = getChar(r, "high");
    }

    return p;
}
