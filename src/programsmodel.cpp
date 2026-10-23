#include "programsmodel.h"

#include "commands.h"
#include "db.h"
#include "enums.h"
#include "lpd8_sysex.h"
#include "midinames.h"
#include "utils.h"

#include <QSortFilterProxyModel>
#include <QStandardItemModel>

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

static const QString pads_group_name = "pads";
static const QString knobs_group_name = "knobs";

// XXX those are kinda nasty, and not data driven...

static const int control_type_pad = 0;
static const int control_type_knob = 1;
// Pads and knobs are handled by the same code, so their id columns must agree.
static_assert(PadColumn::ProgramId == KnobColumn::ProgramId);
static_assert(PadColumn::ControlId == KnobColumn::ControlId);
// The group rows ('pads', 'knobs') have the program id in the same column as
// the programs.
static const int group_program_id_column = ProgramColumn::ProgramId;

ProgramsModel::ProgramsModel(QObject *parent, const QSqlDatabase &db)
    : QAbstractItemModel(parent)
{
    // The database functions only work on the default connection (db.h).
    Q_ASSERT(db.connectionName() == QLatin1StringView(QSqlDatabase::defaultConnection));

    // LUTs, shared with the new models (midinames.h).

    m_lut_default = lpd8::midinames::values();
    m_lut_note = lpd8::midinames::notes();
    m_lut_channel = lpd8::midinames::channels();
    m_lut_toggle = lpd8::midinames::toggleModes();

    m_luts[channel_field_name] = m_lut_channel;
    m_luts[note_field_name] = m_lut_note;
    m_luts[pc_field_name] = m_lut_default;
    m_luts[cc_field_name] = m_lut_default;
    m_luts[toggle_field_name] = m_lut_toggle;
    m_luts[low_field_name] = m_lut_default;
    m_luts[high_field_name] = m_lut_default;

    // Models

    m_empty = new QStandardItemModel(this);

    m_knobs = new QSqlTableModel(this, db);
    m_knobs->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_knobs->setTable("knobs");

    m_pads = new QSqlTableModel(this, db);
    m_pads->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_pads->setTable("pads");

    m_groups = new QStandardItemModel(this);
    m_groups->setHorizontalHeaderLabels({program_id_field_name, name_field_name});

    m_programs = new QSqlTableModel(this, db);
    m_programs->setTable("programs");
    m_programs->setEditStrategy(QSqlTableModel::OnFieldChange);

    connect(m_programs,
            &QSqlTableModel::modelAboutToBeReset,
            this,
            &ProgramsModel::modelAboutToBeReset);

    connect(m_programs, &QSqlTableModel::modelReset, this, &ProgramsModel::modelReset);

    connect(m_programs, &QSqlTableModel::dataChanged, this, &ProgramsModel::dataChanged);

    select();
}

void ProgramsModel::select()
{
    const auto idsList = programIds();
    const QSet<int> ids(idsList.begin(), idsList.end());
    for (const int id : ids) {
        if (!m_groups_proxies.contains(id)) {
            addFilters(id);
        }
    }

    const auto proxyIdsList = m_groups_proxies.keys();
    const QSet<int> toDeleteIds = QSet<int>(proxyIdsList.begin(), proxyIdsList.end()) - ids;
    for (const int id : toDeleteIds) {
        removeFilters(id);
    }

    m_knobs->select();
    m_pads->select();

#ifndef QT_NO_DEBUG
    Q_ASSERT(m_groups_proxies.count() == ids.count());
    Q_ASSERT(m_pads_proxies.count() == ids.count());
    Q_ASSERT(m_knobs_proxies.count() == ids.count());

    for (const int id : ids) {
        Q_ASSERT(m_groups_proxies.contains(id));
        Q_ASSERT(m_pads_proxies.contains(id));
        Q_ASSERT(m_knobs_proxies.contains(id));
        Q_ASSERT(m_groups_proxies[id]->rowCount() == 2);
        Q_ASSERT(m_pads_proxies[id]->rowCount() == sysex::padsCount());
        Q_ASSERT(m_knobs_proxies[id]->rowCount() == sysex::knobsCount());
    }
#endif

    m_programs->select();
}

const QAbstractItemModel *ProgramsModel::modelFromParent(const QModelIndex &parent) const
{
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

int ProgramsModel::columnCount(const QModelIndex & /*parent*/) const
{
    return std::max({m_programs->columnCount(),
                     m_groups->columnCount(),
                     m_pads->columnCount(),
                     m_knobs->columnCount()});
}

int ProgramsModel::rowCount(const QModelIndex &parent) const
{
    const QAbstractItemModel *m = modelFromParent(parent);
    Q_ASSERT(m);

    return m->rowCount();
}

QVariant ProgramsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }
    const QAbstractItemModel *m = model(index);
    Q_ASSERT(m);

    if (hasLut(index)) {
        switch (role) {
        case Qt::DisplayRole:
            return lut(index)[index.data(Qt::EditRole).toInt()];
        case MidiDataRole::MidiValues:
            return lut(index);
        default:
            break;
        }
    }

    return m->data(m->index(index.row(), index.column()), role);
}

bool ProgramsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // Only edits are accepted: the other roles (display text, value lists)
    // are computed from the stored value.
    if (!index.isValid() || role != Qt::EditRole || (flags(index) & Qt::ItemIsEditable) == 0) {
        return false;
    }

    // A value with a lookup table must be one of its entries: 0-127 for
    // MIDI values, 0-15 for channels, 0-1 for toggles. Rejecting it here
    // keeps it out of the undo stack, and tells the caller.
    if (hasLut(index)) {
        bool isInt = false;
        const int v = value.toInt(&isInt);
        if (!isInt || v < 0 || v >= lut(index).size()) {
            return false;
        }
    }

    // Leaving an editor without changing anything is not an edit, and must
    // not add an undo step that does nothing.
    if (index.data(Qt::EditRole) == value) {
        return true;
    }

    QAbstractItemModel *m = model(index);
    Q_ASSERT(m);

    const std::optional<ParameterLocation> location = parameterLocation(index);
    if (!location) {
        return false;
    }
    undoStack()->push(new UpdateParameterCommand(this, *location, value));
    return true;
}

Qt::ItemFlags ProgramsModel::flags(const QModelIndex &index) const
{
    // The invalid index is the root: views ask for its flags too.
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    const QAbstractItemModel *m = model(index);
    Q_ASSERT(m);

    Qt::ItemFlags flags = m->flags(m->index(index.row(), index.column()));

    // QSqlTableModel are Qt::ItemNeverHasChildren by default

    const auto *mp = qobject_cast<const QSortFilterProxyModel *>(m);
    auto *cmp = const_cast<QSortFilterProxyModel *>(mp);
    const bool indexIsInLastChildTables =
            m_pads_proxies.key(cmp) > 0 || m_knobs_proxies.key(cmp) > 0;
    if (!indexIsInLastChildTables) {
        flags &= ~Qt::ItemNeverHasChildren;
    }

    // Do not allow edition of programIds, controlIds and grouping items

    const QString columnName = m->headerData(index.column(), Qt::Horizontal).toString();
    if (columnName == program_id_field_name || columnName == control_id_field_name) {
        flags &= ~Qt::ItemIsEditable;
    }

    if (columnName == toggle_field_name) {
        flags &= ~Qt::ItemIsEnabled;
    }

    return flags;
}

QVariant ProgramsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return m_pads->headerData(section, orientation, role);
}

QModelIndex ProgramsModel::index(int row, int column, const QModelIndex &parent) const
{
    // hasIndex() checks row and column against rowCount() and columnCount():
    // an index must never point outside the model.
    if (!hasIndex(row, column, parent)) {
        return {};
    }
    const QAbstractItemModel *m = modelFromParent(parent);
    Q_ASSERT(m);

    return createIndex(row, column, const_cast<QAbstractItemModel *>(m));
}

QModelIndex ProgramsModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return {};
    }
    const QAbstractItemModel *m = model(child);
    Q_ASSERT(m);

    const auto *const_program_proxy = qobject_cast<const QSortFilterProxyModel *>(m);
    auto *program_proxy = const_cast<QSortFilterProxyModel *>(const_program_proxy);

    // Child is a knob ? Return knob row of matching group model

    int programId = m_knobs_proxies.key(program_proxy);
    if (programId > 0) {
        Q_ASSERT(m_groups_proxies.contains(programId));
        return createIndex(control_type_knob, 0, m_groups_proxies[programId]);
    }

    // Child is a pad ? Return pad row of matching group model

    programId = m_pads_proxies.key(program_proxy);
    if (programId > 0) {
        Q_ASSERT(m_groups_proxies.contains(programId));
        return createIndex(control_type_pad, 0, m_groups_proxies[programId]);
    }

    // Child is group ? Return program row

    programId = m_groups_proxies.key(program_proxy);
    if (programId > 0) {
        QModelIndexList indices =
                m_programs->match(m_programs->index(0, 0), Qt::DisplayRole, programId);
        Q_ASSERT(indices.count() == 1);
        return createIndex(indices[0].row(), 0, m_programs);
    }

    // Child a program ? Return the root (invalid) index

    Q_ASSERT(m == m_programs);
    return {};
}

QAbstractItemModel *ProgramsModel::model(const QModelIndex &index)
{
    return qobject_cast<QAbstractItemModel *>(static_cast<QObject *>(index.internalPointer()));
}

const QAbstractItemModel *ProgramsModel::model(const QModelIndex &index) const
{
    return qobject_cast<QAbstractItemModel *>(static_cast<QObject *>(index.internalPointer()));
}

void ProgramsModel::addFilters(int programId)
{
    Q_ASSERT(!m_groups_proxies.contains(programId));

    // Regexes

    const QRegularExpression regex(QString("^%1$").arg(programId));

    // Groups

    auto *programIdItem = new QStandardItem(QString::number(programId));
    auto *nameItem = new QStandardItem(pads_group_name);
    programIdItem->setEditable(false);
    nameItem->setEditable(false);
    m_groups->appendRow({programIdItem, nameItem});

    programIdItem = new QStandardItem(QString::number(programId));
    nameItem = new QStandardItem(knobs_group_name);
    programIdItem->setEditable(false);
    nameItem->setEditable(false);
    m_groups->appendRow({programIdItem, nameItem});

    auto *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(m_groups);
    proxy->setFilterKeyColumn(group_program_id_column);
    proxy->setFilterRegularExpression(regex);
    m_groups_proxies[programId] = proxy;

    // Pads

    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(m_pads);
    proxy->setFilterKeyColumn(PadColumn::ProgramId);
    proxy->setFilterRegularExpression(regex);
    m_pads_proxies[programId] = proxy;

    connect(proxy,
            &QSortFilterProxyModel::dataChanged,
            this,
            [this, programId](const QModelIndex &topLeft,
                              const QModelIndex &bottomRight,
                              const QVector<int> &roles) {
                const QModelIndex programIndex = this->programIndex(programId);
                const QModelIndex parent = this->padsParentIndex(programIndex);
                const QModelIndex tl = index(topLeft.row(), topLeft.column(), parent);
                const QModelIndex br = index(bottomRight.row(), bottomRight.column(), parent);
                emit dataChanged(tl, br, roles);
            });

    // Knobs

    proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(m_knobs);
    proxy->setFilterKeyColumn(KnobColumn::ProgramId);
    proxy->setFilterRegularExpression(regex);
    m_knobs_proxies[programId] = proxy;

    connect(proxy,
            &QSortFilterProxyModel::dataChanged,
            this,
            [this, programId](const QModelIndex &topLeft,
                              const QModelIndex &bottomRight,
                              const QVector<int> &roles) {
                const QModelIndex programIndex = this->programIndex(programId);
                const QModelIndex parent = this->knobsParentIndex(programIndex);
                const QModelIndex tl = index(topLeft.row(), topLeft.column(), parent);
                const QModelIndex br = index(bottomRight.row(), bottomRight.column(), parent);
                emit dataChanged(tl, br, roles);
            });
}

void ProgramsModel::removeFilters(int programId)
{
    Q_ASSERT(m_groups_proxies.contains(programId));

    const QString findItemsText(QString::number(programId));

    // Groups

    m_groups_proxies.take(programId)->deleteLater();

    QList<QStandardItem *> items = m_groups->findItems(findItemsText);
    Q_ASSERT(items.count() == 2);
    Q_ASSERT((items[1]->row() - items[0]->row()) == 1); // The two indices are contiguous
    m_groups->removeRows(items[0]->row(), static_cast<int>(items.count()));

    // Pads

    m_pads_proxies.take(programId)->deleteLater();

    // Knobs

    m_knobs_proxies.take(programId)->deleteLater();
}

bool ProgramsModel::hasLut(const QModelIndex &index) const
{
    const QAbstractItemModel *m = model(index);
    Q_ASSERT(m);

    const QString name = m->headerData(index.column(), Qt::Horizontal).toString();
    return m_luts.contains(name);
}

QStringList ProgramsModel::lut(const QModelIndex &index) const
{
    Q_ASSERT(hasLut(index));

    const QAbstractItemModel *m = model(index);
    Q_ASSERT(m);

    const QString name = m->headerData(index.column(), Qt::Horizontal).toString();
    return m_luts[name];
}

QModelIndex ProgramsModel::padsParentIndex(const QModelIndex &programIndex) const
{
    Q_ASSERT(programIndex.isValid());

    Q_ASSERT(model(programIndex) == m_programs);

    const QModelIndex program = index(programIndex.row(), ProgramColumn::ProgramId);
    const QModelIndex pads = index(control_type_pad, group_program_id_column, program);

#ifndef QT_NO_DEBUG
    const QAbstractItemModel *m = model(pads);
    Q_ASSERT(m);
    const int programId = pads.data().toInt();
    Q_ASSERT(m_pads_proxies.contains(programId));
    Q_ASSERT(m == m_groups_proxies[programId]);
#endif

    return pads;
}

QModelIndex ProgramsModel::knobsParentIndex(const QModelIndex &programIndex) const
{
    Q_ASSERT(programIndex.isValid());

    Q_ASSERT(model(programIndex) == m_programs);

    const QModelIndex program = index(programIndex.row(), ProgramColumn::ProgramId);
    const QModelIndex knobs = index(control_type_knob, group_program_id_column, program);

#ifndef QT_NO_DEBUG
    const QAbstractItemModel *m = model(knobs);
    Q_ASSERT(m);
    const int programId = knobs.data().toInt();
    Q_ASSERT(m_knobs_proxies.contains(programId));
    Q_ASSERT(m == m_groups_proxies[programId]);
#endif

    return knobs;
}

QAbstractItemModel *ProgramsModel::padsHeaderModel() const
{
    Q_ASSERT(m_pads);

    return m_pads;
}

QAbstractItemModel *ProgramsModel::knobsHeaderModel() const
{
    Q_ASSERT(m_knobs);

    return m_knobs;
}

QModelIndex ProgramsModel::programIndex(int programId) const
{
    Q_ASSERT(m_programs);

    for (int row = 0; row < m_programs->rowCount(); ++row) {
        const QModelIndex idx = index(row, ProgramColumn::ProgramId);
        if (idx.data(Qt::EditRole).toInt() == programId) {
            return idx;
        }
    }

    return {};
}

namespace {

// The program id of the proxy 'model' in 'proxies', or -1.
int programIdOf(const QHash<int, QSortFilterProxyModel *> &proxies, const QAbstractItemModel *model)
{
    for (auto it = proxies.cbegin(); it != proxies.cend(); ++it) {
        if (it.value() == model) {
            return it.key();
        }
    }
    return -1;
}

} // namespace

std::optional<ParameterLocation> ProgramsModel::parameterLocation(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return std::nullopt;
    }
    const QAbstractItemModel *m = model(index);

    if (m == m_programs) {
        const int programId =
                m->index(index.row(), ProgramColumn::ProgramId).data(Qt::EditRole).toInt();
        return ParameterLocation{.programId = programId,
                                 .group = ParameterLocation::Group::Program,
                                 .controlId = 0,
                                 .column = index.column()};
    }

    const auto group = [&]() -> std::optional<ParameterLocation::Group> {
        if (programIdOf(m_pads_proxies, m) != -1) {
            return ParameterLocation::Group::Pads;
        }
        if (programIdOf(m_knobs_proxies, m) != -1) {
            return ParameterLocation::Group::Knobs;
        }
        return std::nullopt; // the 'pads' and 'knobs' group rows are not parameters
    }();
    if (!group) {
        return std::nullopt;
    }
    const auto &proxies =
            *group == ParameterLocation::Group::Pads ? m_pads_proxies : m_knobs_proxies;
    const int controlId = m->index(index.row(), PadColumn::ControlId).data(Qt::EditRole).toInt();
    return ParameterLocation{.programId = programIdOf(proxies, m),
                             .group = *group,
                             .controlId = controlId,
                             .column = index.column()};
}

QModelIndex ProgramsModel::parameterIndex(const ParameterLocation &location) const
{
    const QModelIndex program = programIndex(location.programId);
    if (!program.isValid()) {
        return {};
    }

    switch (location.group) {
    case ParameterLocation::Group::Program:
        return index(program.row(), location.column);
    case ParameterLocation::Group::Pads:
    case ParameterLocation::Group::Knobs: {
        const QModelIndex group = location.group == ParameterLocation::Group::Pads
                ? padsParentIndex(program)
                : knobsParentIndex(program);
        for (int row = 0; row < rowCount(group); ++row) {
            const QModelIndex controlId = index(row, PadColumn::ControlId, group);
            if (controlId.data(Qt::EditRole).toInt() == location.controlId) {
                return index(row, location.column, group);
            }
        }
        return {};
    }
    }
    return {};
}

bool ProgramsModel::writeParameter(const QModelIndex &index, const QVariant &value)
{
    QAbstractItemModel *m = model(index);
    if (m == nullptr) {
        return false;
    }
    return m->setData(m->index(index.row(), index.column()), value, Qt::EditRole);
}

int ProgramsModel::createProgram(const QString &programName, const QByteArray &sysex, int programId)
{
    const int ret = ::createProgram(programName, sysex, programId);
    if (ret != -1) {
        this->select();
    }
    return ret;
}

bool ProgramsModel::deleteProgram(int programId)
{
    const bool ret = ::deleteProgram(programId);
    if (ret) {
        this->select();
    }
    return ret;
}

QString ProgramsModel::programName(int programId) const
{
    return ::programName(programId);
}

QByteArray ProgramsModel::programSysex(int programId) const
{
    return ::programSysex(programId);
}

bool ProgramsModel::updateProgramFromSysex(int programId, const QByteArray &sysex)
{
    const bool ret = fromSysex(programId, sysex);
    if (ret) {
        this->select();
    }
    return ret;
}

int selectedProgramId(QItemSelectionModel *model)
{
    Q_ASSERT(model);

    const QAbstractItemModel *m = model->model();
    Q_ASSERT(m);

    const QModelIndex curSelIdx = model->currentIndex();
    const QModelIndex curProjIdx = m->index(curSelIdx.row(), 0);
    return curProjIdx.data(Qt::EditRole).toInt();
}

ProgramIdSelectionRestorer::ProgramIdSelectionRestorer(QItemSelectionModel *model, QObject *parent)
    : QObject(parent)
    , m_model(model)
{
    Q_ASSERT(m_model);

    store();
}

void ProgramIdSelectionRestorer::store()
{
    m_program_id = selectedProgramId();
}

void ProgramIdSelectionRestorer::restore()
{
    const ProgramsModel *m = qobject_cast<ProgramsModel *>(m_model->model());
    Q_ASSERT(m);

    if (m->programIndex(m_program_id).isValid()) {
        selectProgramId(m_program_id);
    }
}

int ProgramIdSelectionRestorer::selectedProgramId() const
{
    return ::selectedProgramId(m_model);
}

void ProgramIdSelectionRestorer::selectProgramId(int programId)
{
    const ProgramsModel *m = qobject_cast<ProgramsModel *>(m_model->model());
    Q_ASSERT(m);

    const QModelIndex programIdx = m->programIndex(programId);
    Q_ASSERT(programIdx.isValid());

    m_model->setCurrentIndex(programIdx,
                             QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}
