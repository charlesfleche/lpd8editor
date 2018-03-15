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

// XXX those are kinda nasty, and not data driven...

static const int control_type_pad = 0;
static const int control_type_knob = 1;
static const int program_id_column_index = 0;
static const int program_pads_count = 8;
static const int program_knobs_count = 8;


QString programIdFilter(int projectId) {
    return QString("%1='%2'").arg(program_id_field_name).arg(projectId);
}

bool deleteRecordsForProgramId(QSqlTableModel* model, int projectId, int expectedRemove) {
    Q_CHECK_PTR(model);

    QModelIndexList indices = model->match(
        model->index(0, 0),
        Qt::DisplayRole,
        projectId
    );

    QSet<int> rows;
    for (auto it = indices.begin() ; it != indices.end() ; ++it) {
        rows << (*it).row();
    }

    Q_ASSERT(rows.count() == expectedRemove);
    Q_ASSERT(*std::max_element(rows.begin(), rows.end()) - *std::min_element(rows.begin(), rows.end()) == 0);

    model->removeRows(*std::min_element(rows.begin(), rows.end()), rows.count());

    if (model->submitAll()) {
        model->select();
        return true;
    }
    return false;
}


ProgramsModel::ProgramsModel(QObject *parent) :
    QAbstractItemModel(parent),
    m_groups(Q_NULLPTR),
    m_pads(Q_NULLPTR),
    m_knobs(Q_NULLPTR),
    m_programs(Q_NULLPTR),
    m_empty(Q_NULLPTR)
{
    m_empty = new QStandardItemModel(this);

    m_knobs = new QSqlTableModel(this);
    m_knobs->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_knobs->setTable("knobs");
    m_knobs->select();

    m_pads = new QSqlTableModel(this);
    m_pads->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_pads->setTable("pads");
    m_pads->select();

    m_groups = new QStandardItemModel(this);
    m_groups->setHorizontalHeaderLabels({program_id_field_name, name_field_name});

    m_programs = new QSqlTableModel(this);
    m_programs->setTable("programs");
    m_programs->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_programs->select();

    for (int i = 0 ; i < m_programs->rowCount() ; ++i) {
        addFilters(m_programs->record(i).value(program_id_column_index).toInt());
    }

    qDebug() << "this" << this;
    qDebug() << "m_programs" << m_programs;
    qDebug() << "m_groups" << m_groups;
    for (auto it = m_groups_proxies.keyBegin() ; it != m_groups_proxies.keyEnd() ; ++it) {
        QSortFilterProxyModel* m = m_groups_proxies[*it];
        qDebug() << "m_groups_proxies" << *it << m << m->rowCount() ;
    }
    for (auto it = m_pads_proxies.keyBegin() ; it != m_pads_proxies.keyEnd() ; ++it) {
        QSortFilterProxyModel* m = m_pads_proxies[*it];
        qDebug() << "m_pads_proxies" << *it << m << m->rowCount();
    }
    for (auto it = m_knobs_proxies.keyBegin() ; it != m_knobs_proxies.keyEnd() ; ++it) {
        QSortFilterProxyModel* m = m_knobs_proxies[*it];
        qDebug() << "m_knobs_proxies" << *it << m << m->rowCount();
    }
    qDebug() << "m_empty" << m_empty;

    connect(
        m_programs,
        &QSqlTableModel::rowsInserted,
        this,
        &ProgramsModel::rowsInserted
    );

    // QSqlTableModel does not emit rowsDeleted signals,
    // but reset it's model after the necessary select()
    // after removing rows

    connect(
        m_programs,
        &QSqlTableModel::modelAboutToBeReset,
        this,
        &ProgramsModel::modelAboutToBeReset
    );

    connect(
        m_programs,
        &QSqlTableModel::modelReset,
        this,
        &ProgramsModel::modelReset
    );
}

const QAbstractItemModel* ProgramsModel::modelFromParent(const QModelIndex &parent) const {
    QModelIndex idx = parent;
    int lvl = 0;
    while (idx.isValid()) {
        ++lvl;
        idx = idx.parent();
    }

    // parent is the top level invalid index

    if (lvl == 0) {
        return m_programs;
    }

    // Only first column can have children

    if (parent.column() != 0) {
        return m_empty;
    }

    const int programId = parent.data().toInt();

    // parent is in m_programs: returns a group proxy

    if (lvl == 1) {
        Q_ASSERT(m_groups_proxies.contains(programId));
        return m_groups_proxies[programId];
    }

    // parent is a group proxy

    if (lvl == 2) {
        switch (parent.row()) {
        case control_type_pad:
            Q_ASSERT(m_pads_proxies.contains(programId));
            return m_pads_proxies[programId];
        case control_type_knob:
            Q_ASSERT(m_knobs_proxies.contains(programId));
            return m_knobs_proxies[programId];
        default:
            break;
        }
    }

    return m_empty;
}

int ProgramsModel::columnCount(const QModelIndex &) const {
    Q_CHECK_PTR(m_programs);
    Q_CHECK_PTR(m_groups);
    Q_CHECK_PTR(m_pads);
    Q_CHECK_PTR(m_knobs);

    return std::max({
        m_programs->columnCount(),
        m_groups->columnCount(),
        m_pads->columnCount(),
        m_knobs->columnCount()
    });
}

int ProgramsModel::rowCount(const QModelIndex &parent) const {
    const QAbstractItemModel* m = modelFromParent(parent);
    Q_CHECK_PTR(m);

    int ret = m->rowCount();
    qDebug() << "rowCount" << parent << m << ret;
    return ret;
}

QVariant ProgramsModel::data(const QModelIndex &index, int role) const {
    const QAbstractItemModel* m = model(index);
    Q_CHECK_PTR(m);

    QModelIndex idx = m->index(index.row(), index.column());
    QVariant ret = m->data(idx, role);

    if (role == Qt::DisplayRole) {
        qDebug() << "data" << index << role << m << ret;
    }
    return ret;
}

QModelIndex ProgramsModel::index(int row, int column, const QModelIndex &parent) const {
    const QAbstractItemModel* m = modelFromParent(parent);
    Q_CHECK_PTR(m);

//    qDebug() << "index" << row << column << parent << m->index(row, column, parent);
    return createIndex(row, column, const_cast<QAbstractItemModel*>(m));
}


QModelIndex ProgramsModel::parent(const QModelIndex &child) const {
    const QAbstractItemModel* m = model(child);
    Q_CHECK_PTR(m);

    QModelIndex ret;

    const QSortFilterProxyModel* const_program_proxy = qobject_cast<const QSortFilterProxyModel*>(m);
    QSortFilterProxyModel* program_proxy = const_cast<QSortFilterProxyModel*>(const_program_proxy);

    // Child is a know ? Return knob row of matching group model

    int programId = m_knobs_proxies.key(program_proxy);
    if (programId > 0) {
        Q_ASSERT(m_groups_proxies.contains(programId));
        ret = createIndex(control_type_knob, 0, m_groups_proxies[programId]);
        goto end;
    }

    // Child is a pad ? Return pad row of matching group model

    programId = m_pads_proxies.key(program_proxy);
    if (programId > 0) {
        Q_ASSERT(m_groups_proxies.contains(programId));
        ret = createIndex(control_type_pad, 0, m_groups_proxies[programId]);
        goto end;
    }

    // Child is group ? Return program row

    programId = m_groups_proxies.key(program_proxy);
    if (programId > 0) {
        QModelIndexList indices = m_programs->match(
            m_programs->index(0, 0),
            Qt::DisplayRole,
            programId
        );
        Q_ASSERT(indices.count() == 1);
        ret = createIndex(indices[0].row(), 0, m_programs);
        goto end;
    }

end:
//    qDebug() << "parent" << child << ret;
    return ret;
}

int ProgramsModel::createProgram(const QString &name, const QByteArray &sysex) {
    Q_CHECK_PTR(m_programs);
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

    QSqlRecord r(m_programs->record());
    r.remove(r.indexOf(program_id_field_name));
    r.setValue(name_field_name, name);
    s >> v;
    r.setValue(channel_field_name, v);

    if (!m_programs->insertRecord(-1, r)) {
         qWarning() << "Cannot create program:" << m_programs->lastError().text();
         return -1;
    }

    const int programId = m_programs->query().lastInsertId().toInt();

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
             qWarning() << "Cannot create pad" << i << "for program:" << m_pads->lastError().text();
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
             qWarning() << "Cannot create pad" << i << "for program:" << m_knobs->lastError().text();
             return -1;
        }
    }

    addFilters(programId);

    return programId;
}

bool ProgramsModel::deleteProgram(int programId) {
    QScopedPointer<QSqlTableModel> m(new QSqlTableModel());

    if (!deleteRecordsForProgramId(m_programs, programId, 1)) {
        qWarning() << "Failed deleting program:" << programId;
        return false;
    }

//    removeFilters(programId);

//    if (!deleteRecordsForProgramId(m.data(), m_knobs->tableName(), programId)) {
//        qWarning() << "Failed deleting knobs for program:" << programId;
//        return false;
//    }
//    m_knobs->select();

//    if (!deleteRecordsForProgramId(m.data(), m_pads->tableName(), programId)) {
//        qWarning() << "Failed deleting pads for program:" << programId;
//        return false;
//    }
//    m_pads->select();

    return true;
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

    removeFilters(programId);

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
    return qobject_cast<QAbstractItemModel*>(static_cast<QObject*>(index.internalPointer()));
}

const QAbstractItemModel* ProgramsModel::model(const QModelIndex &index) const {
    return qobject_cast<QAbstractItemModel*>(static_cast<QObject*>(index.internalPointer()));
}

void ProgramsModel::addFilters(int programId) {
    Q_CHECK_PTR(m_groups);
    Q_ASSERT(!m_groups_proxies.contains(programId));

    // Regexes

    const QRegExp regex(QString("^%1$").arg(programId));

    // Groups

    m_groups->appendRow({new QStandardItem(QString::number(programId)), new QStandardItem(QString("pads%1").arg(programId))});
    m_groups->appendRow({new QStandardItem(QString::number(programId)), new QStandardItem(QString("knobs%1").arg(programId))});

    QSortFilterProxyModel* proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(m_groups);
    proxy->setFilterKeyColumn(program_id_column_index);
    proxy->setFilterRegExp(regex);
    Q_ASSERT(proxy->rowCount() == 2);
    m_groups_proxies[programId] = proxy;

    // Pads

    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(m_pads);
    proxy->setFilterKeyColumn(program_id_column_index);
    proxy->setFilterRegExp(regex);
    Q_ASSERT(proxy->rowCount() == program_pads_count);
    m_pads_proxies[programId] = proxy;

    // Knobs

    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(m_knobs);
    proxy->setFilterKeyColumn(program_id_column_index);
    proxy->setFilterRegExp(regex);
    Q_ASSERT(proxy->rowCount() == program_knobs_count);
    m_knobs_proxies[programId] = proxy;
}

void ProgramsModel::removeFilters(int programId) {
    Q_CHECK_PTR(m_groups);
    Q_ASSERT(m_groups_proxies.contains(programId));

    const QString findItemsText(QString::number(programId));

    // Groups

    m_groups_proxies.take(programId)->deleteLater();

    QList<QStandardItem*> items = m_groups->findItems(findItemsText);
    Q_ASSERT(items.count() == 2);
    Q_CHECK_PTR(items[0]);
    Q_CHECK_PTR(items[1]);
    Q_ASSERT((items[1]->row() - items[0]->row()) == 1); // The two indices are contiguous
    m_groups->removeRows(items[0]->row(), items.count());

    // Pads

    m_pads_proxies.take(programId)->deleteLater();

    // Knobs

    m_knobs_proxies.take(programId)->deleteLater();
}
