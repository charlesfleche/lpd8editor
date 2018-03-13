#include "programsmodel.h"

#include "lpd8_sysex.h"

#include <QDataStream>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
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
    QAbstractItemModel(parent),
    m_groups(Q_NULLPTR),
    m_pads(Q_NULLPTR),
    m_knobs(Q_NULLPTR),
    m_programs(Q_NULLPTR)
{
    m_knobs = new QSqlTableModel(this);
    m_knobs->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_knobs->setTable("knobs");

    m_pads = new QSqlTableModel(this);
    m_pads->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_pads->setTable("pads");
    m_pads->select();

    m_groups = new QStandardItemModel(this);
    QStandardItem* it = new QStandardItem("Pads");
//    it->setEditable(false);
    m_groups->appendRow(it);
    it = new QStandardItem("Knobs");
//    it->setEditable(false);
    m_groups->appendRow(it);

    m_programs = new QSqlTableModel(this);
    m_programs->setTable("programs");
    m_programs->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_programs->select();
}

int ProgramsModel::columnCount(const QModelIndex &parent) const {
    const QAbstractItemModel* m = modelFromParent(parent);
    Q_CHECK_PTR(m);

    qDebug() << "columnCount" << parent << m->columnCount(parent);
    return m->columnCount(parent);
}

QVariant ProgramsModel::data(const QModelIndex &index, int role) const {
    const QAbstractItemModel* m = model(index);
    Q_CHECK_PTR(m);

//    QVariant ret = index.data(role);
    QModelIndex idx = m->index(index.row(), index.column());
    QVariant ret = m->data(idx, role);

    qDebug() << "data" << index << role << ret;
    return ret;
}

QModelIndex ProgramsModel::index(int row, int column, const QModelIndex &parent) const {
    const QAbstractItemModel* m = modelFromParent(parent);
    Q_CHECK_PTR(m);

    qDebug() << "index" << row << column << parent << m->index(row, column, parent);
//    return m->index(row, column, parent);
    return createIndex(row, column, const_cast<QAbstractItemModel*>(m));
}

//QModelIndex ProgramsModel::createIndex(int row, int column, void *ptr) const {
//    const QAbstractItemModel* m = modelFromParent(parent);
//    Q_CHECK_PTR(m);

//    qDebug() << "index" << row << column << parent << m->index(row, column, parent);
//    return m->index(row, column, parent);
//}

//QModelIndex createIndex(int row, int column, quintptr id) const;

//Qt::ItemFlags ProgramsModel::flags(const QModelIndex &index) const {
//    const QAbstractItemModel* m = model(index);
//    Q_CHECK_PTR(m);

////    Qt::ItemFlags ret = index.flags();
//    Qt::ItemFlags ret = m->flags(index);

//    qDebug() << "flags" << index << ret;
//    return ret;
//}

//bool ProgramsModel::hasChildren(const QModelIndex &parent) const {
//    const QAbstractItemModel* m = model(parent);
//    Q_CHECK_PTR(m);

//    qDebug() << "hasChildren" << parent << m->hasChildren(parent);
//    return m->hasChildren(parent);
//}

QModelIndex ProgramsModel::parent(const QModelIndex &child) const {
    const QAbstractItemModel* m = model(child);
    Q_CHECK_PTR(m);

//    QModelIndex ret = child.parent();
    QModelIndex ret = m->parent(child);

    qDebug() << "parent" << child << ret;
    return ret;
}

int ProgramsModel::rowCount(const QModelIndex &parent) const {
    const QAbstractItemModel* m = modelFromParent(parent);
    Q_CHECK_PTR(m);

    qDebug() << "rowCount" << parent << m->rowCount(parent);
    return m->rowCount(parent);
}

//bool ProgramsModel::canFetchMore(const QModelIndex &parent) const {
//    const QAbstractItemModel* m = modelFromParent(parent);
//    Q_CHECK_PTR(m);

//    qDebug() << "canFetchMore" << parent << m->canFetchMore(parent);
//    return m->canFetchMore(parent);
//}

//void ProgramsModel::fetchMore(const QModelIndex &parent) {
//    QAbstractItemModel* m = modelFromParent(parent);
//    Q_CHECK_PTR(m);

//    qDebug() << "fetchMore" << parent;
//    m->fetchMore(parent);
//}

//bool ProgramsModel::setData(const QModelIndex &index, const QVariant &value, int role) {
//    QAbstractItemModel* m = model(index);
//    Q_CHECK_PTR(m);

//    return m->setData(index, value, role);
//}

int ProgramsModel::createProgram(const QString &name, const QByteArray &sysex) {
#if 0
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
#else
    Q_UNUSED(name);
    Q_UNUSED(sysex);
    return -1;
#endif
}

bool ProgramsModel::deleteProgram(int programId) {
#if 0
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
#else
    Q_UNUSED(programId);
    return false;
#endif
}

QString ProgramsModel::name(int programId) const {
#if 0
    return record(programRow(programId)).value(name_field_name).toString();
#else
    Q_UNUSED(programId);
    return "NIY";
#endif
}

QByteArray ProgramsModel::sysex(int programId) const {
#if 0
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
#else
    Q_UNUSED(programId);
    return QByteArray();
#endif
}

int ProgramsModel::programRow(int programId) const {
#if 0
    const QModelIndexList indices(match(index(0, 0), Qt::DisplayRole, programId));
    Q_ASSERT(indices.count() == 1);
    return indices[0].row();
#else
    Q_UNUSED(programId);
    return -1;
#endif
}

QAbstractItemModel* ProgramsModel::model(const QModelIndex &index)  {
    Q_UNUSED(index);
//    return m_groups;
    return m_programs;
//    return modelFromParent(index.parent());
}

const QAbstractItemModel* ProgramsModel::model(const QModelIndex &index) const {
    Q_UNUSED(index);
//    return m_groups;
    return m_programs;
//    return modelFromParent(index.parent());
}

QAbstractItemModel* ProgramsModel::modelFromParent(const QModelIndex &parent)  {
    Q_UNUSED(parent);
//    return m_groups;
    return m_programs;
//    return parent.isValid() ? qobject_cast<QAbstractItemModel*>(m_groups) : this;
}

const QAbstractItemModel* ProgramsModel::modelFromParent(const QModelIndex &parent) const {
    Q_UNUSED(parent);
//    return m_groups;
    return m_programs;
//    return parent.isValid() ? qobject_cast<QAbstractItemModel*>(m_groups) : this;
}
