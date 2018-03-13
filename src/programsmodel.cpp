#include "programsmodel.h"

#include "lpd8_sysex.h"

#include <QDataStream>
#include <QSortFilterProxyModel>
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
#include <QSqlQuery>

#include <QtDebug>

static const QString cc_field_name("cc");
static const QString channel_field_name("channel");
static const QString control_id_field_name("controlId");
static const QString high_field_name("high");
static const QString low_field_name("low");
static const QString name_field_name("name");
static const QString note_field_name("note");
static const QString pc_field_name("pc");
static const QString program_id_field_name("programId");
static const QString toggle_field_name("toggle");


QString programIdFilter(int projectId) {
    return QString("%1='%2'").arg(program_id_field_name).arg(projectId);
}

bool deleteRecordsForProgramId(QSqlTableModel* model, const QString& table, int projectId) {
    Q_CHECK_PTR(model);

    model->setTable(table);
    model->setFilter(programIdFilter(projectId));
    model->select();
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->removeRows(0, model->rowCount());
    return model->submitAll();
}


ProgramsModel::ProgramsModel(QObject *parent) :
    QSqlTableModel(parent),
    m_pads(Q_NULLPTR),
    m_knobs(Q_NULLPTR)
{
    setTable("programs");
    setEditStrategy(QSqlTableModel::OnFieldChange);
    select();

    m_pads = new QSqlTableModel(this);
    m_pads->setTable("pads");

    m_knobs = new QSqlTableModel(this);
    m_knobs->setTable("knobs");
}

QVariant ProgramsModel::data(const QModelIndex &idx, int role) const {
    return QSqlTableModel::data(idx, role);
}

Qt::ItemFlags ProgramsModel::flags(const QModelIndex &index) const {
    return QSqlTableModel::flags(index);
}

int ProgramsModel::rowCount(const QModelIndex &parent) const {
    return QSqlTableModel::rowCount(parent);
}

bool ProgramsModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    return QSqlTableModel::setData(index, value, role);
}

int ProgramsModel::createProgram(const QString &name, const QByteArray &sysex) {
    Q_CHECK_PTR(m_pads);
    Q_CHECK_PTR(m_knobs);

    QDataStream s(sysex);
    qint8 v = 0;

    // Header

    s >> v; // Sysex Start
    s >> v; // Manufacturer
    s >> v; // Model byte 1
    s >> v; // Model byte 2
    s >> v; // Opcode set program: byte1
    s >> v; // Opcode set program: byte2
    s >> v; // Opcode set program: byte3
    s >> v; // Program ID

    // Program

    QSqlRecord r(record());
    r.remove(r.indexOf(program_id_field_name));
    r.setValue(name_field_name, name);
    s >> v;
    r.setValue(channel_field_name, v);

    if (!insertRecord(-1, r)) {
         qWarning() << "Cannot create program:" << lastError().text();
         return -1;
    }

    const int programId = query().lastInsertId().toInt();

    // Pads

    for (int i = 0; i < sysex::padsCount(); ++i) {
        QSqlRecord r(m_pads->record());

        r.setValue(program_id_field_name, programId);
        r.setValue(control_id_field_name, i);

        s >> v;
        r.setValue(note_field_name, v);

        s >> v;
        r.setValue(pc_field_name, v);

        s >> v;
        r.setValue(cc_field_name, v);

        s >> v;
        r.setValue(toggle_field_name, v);

        if (!m_pads->insertRecord(-1, r)) {
             qWarning() << "Cannot create pad" << i << "for program:" << lastError().text();
             return -1;
        }
    }

    // Knobs

    for (int i = 0; i < sysex::knobsCount(); ++i) {
        QSqlRecord r(m_knobs->record());

        r.setValue(program_id_field_name, programId);
        r.setValue(control_id_field_name, i);

        s >> v;
        r.setValue(cc_field_name, v);

        s >> v;
        r.setValue(low_field_name, v);

        s >> v;
        r.setValue(high_field_name, v);

        if (!m_knobs->insertRecord(-1, r)) {
             qWarning() << "Cannot create pad" << i << "for program:" << lastError().text();
             return -1;
        }
    }

    return programId;
}

bool ProgramsModel::deleteProgram(int programId) {
    QScopedPointer<QSqlTableModel> m(new QSqlTableModel());

    if (!deleteRecordsForProgramId(m.data(), tableName(), programId)) {
        qWarning() << "Failed deleting program:" << programId;
        return false;
    }

    if (!deleteRecordsForProgramId(m.data(), m_pads->tableName(), programId)) {
        qWarning() << "Failed deleting pads for program:" << programId;
        return false;
    }

    if (!deleteRecordsForProgramId(m.data(), m_knobs->tableName(), programId)) {
        qWarning() << "Failed deleting knobs for program:" << programId;
        return false;
    }

    m_knobs->select();
    m_pads->select();
    select();

    return true;
}

QString ProgramsModel::name(int programId) const {
    return record(programRow(programId)).value(name_field_name).toString();
}

QByteArray ProgramsModel::sysex(int programId) const {
    Q_CHECK_PTR(m_pads);
    Q_CHECK_PTR(m_knobs);

    QByteArray ret;
    sysex::addProgramHeader(ret, 1);

    QSqlRecord r;
    QScopedPointer<QSqlTableModel> m(new QSqlTableModel());
    const QString filter(QString("%1='%2'").arg(program_id_field_name).arg(programId));

    // Program

    m->setTable(tableName());
    m->setFilter(filter);
    m->select();
    Q_ASSERT(m->rowCount() == 1);

    r = m->record(0);

    ret += r.value(channel_field_name).toChar();

    // Pads

    m->setTable(m_pads->tableName());
    m->setFilter(filter);
    m->setSort(m_pads->fieldIndex(control_id_field_name), Qt::AscendingOrder);
    m->select();
    Q_ASSERT(m->rowCount() == sysex::padsCount());

    for (int i = 0 ; i < m->rowCount() ; ++i) {
        r = m->record(i);
        ret += r.value(note_field_name).toChar();
        ret += r.value(pc_field_name).toChar();
        ret += r.value(cc_field_name).toChar();
        ret += r.value(toggle_field_name).toChar();
    }

    // Knobs

    m->setTable(m_knobs->tableName());
    m->setFilter(filter);
    m->setSort(m_knobs->fieldIndex(control_id_field_name), Qt::AscendingOrder);
    m->select();
    Q_ASSERT(m->rowCount() == sysex::knobsCount());

    for (int i = 0 ; i < m->rowCount() ; ++i) {
        r = m->record(i);
        ret += r.value(cc_field_name).toChar();
        ret += r.value(low_field_name).toChar();
        ret += r.value(high_field_name).toChar();
    }

    // Footer

    sysex::addFooter(ret);

    return ret;
}

int ProgramsModel::programRow(int programId) const {
    const QModelIndexList indices(match(index(0, 0), Qt::DisplayRole, programId));
    Q_ASSERT(indices.count() == 1);
    return indices[0].row();
}
