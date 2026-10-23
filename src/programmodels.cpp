#include "programmodels.h"

#include "enums.h"
#include "exactinteger.h"
#include "midinames.h"
#include "programstore.h"
#include "storecommands.h"

#include <QUndoStack>

namespace lpd8 {

namespace {

// Pushes what a factory made. A no-op (nullptr) is accepted and pushes
// nothing; a refusal is refused, so the view shows the stored value again.
bool push(QUndoStack &stack, CommandOrRefusal made)
{
    if (!made) {
        return false;
    }
    if (*made) {
        stack.push(made->release());
    }
    return true;
}

// A value from an editor, as an integer from 0 to 'max'. The same rule as
// for stored values (exactinteger.h): only an integer, never a cast, since
// QVariant::toInt() wraps and rounds.
std::optional<quint8> editorValue(const QVariant &value, int max)
{
    return exactInteger(value, max).transform([](int v) { return static_cast<quint8>(v); });
}

} // namespace

// --- ProgramListModel

ProgramListModel::ProgramListModel(ProgramStore &store, QUndoStack &stack, QObject *parent)
    : QAbstractTableModel(parent)
    , m_store(store)
    , m_stack(stack)
{
    // The store's order is the model's order, and its signals give the row.
    connect(&store, &ProgramStore::programAboutToBeAdded, this, [this](int, int row) {
        beginInsertRows({}, row, row);
    });
    connect(&store, &ProgramStore::programAdded, this, [this] { endInsertRows(); });
    connect(&store, &ProgramStore::programAboutToBeRemoved, this, [this](int, int row) {
        beginRemoveRows({}, row, row);
    });
    connect(&store, &ProgramStore::programRemoved, this, [this] { endRemoveRows(); });
    connect(&store, &ProgramStore::nameChanged, this, [this](int, int row) {
        const QModelIndex name = index(row, NameColumn);
        emit dataChanged(name, name, {Qt::DisplayRole, Qt::EditRole});
    });
    // The list only shows the channel of a program's content. This
    // over-notifies on purpose: programChanged doesn't say what changed, so a
    // pad-only edit also signals the channel cell. Knowing would mean keeping
    // a copy of every channel, a mirror of the store that these models exist
    // to avoid, and a view only repaints that one cell.
    connect(&store, &ProgramStore::programChanged, this, [this](int, int row) {
        const QModelIndex channel = index(row, ChannelColumn);
        emit dataChanged(channel, channel, {Qt::DisplayRole, Qt::EditRole});
    });
    connect(&store, &ProgramStore::programsAboutToBeReset, this, [this] { beginResetModel(); });
    connect(&store, &ProgramStore::programsReset, this, [this] { endResetModel(); });
}

int ProgramListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_store.programs().size());
}

int ProgramListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ProgramListModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }
    const StoredProgram &stored = m_store.programs().at(index.row());
    if (role == IdRole) {
        return stored.id;
    }
    switch (index.column()) {
    case NameColumn:
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            return stored.name;
        }
        break;
    case ChannelColumn:
        if (role == Qt::EditRole) {
            return stored.program.channel;
        }
        if (role == Qt::DisplayRole) {
            return midinames::channels().at(stored.program.channel);
        }
        if (role == MidiDataRole::MidiValues) {
            return midinames::channels();
        }
        break;
    default:
        break;
    }
    return {};
}

bool ProgramListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid) || role != Qt::EditRole) {
        return false;
    }
    const StoredProgram &stored = m_store.programs().at(index.row());
    switch (index.column()) {
    case NameColumn:
        if (value.typeId() != QMetaType::QString) {
            return false;
        }
        return push(m_stack, makeRenameCommand(m_store, stored.id, value.toString()));
    case ChannelColumn: {
        const auto channel = editorValue(value, midi_channel_max);
        if (!channel) {
            return false;
        }
        Program program = stored.program;
        program.channel = *channel;
        return push(m_stack, makeSetProgramCommand(m_store, stored.id, program));
    }
    default:
        return false;
    }
}

Qt::ItemFlags ProgramListModel::flags(const QModelIndex &index) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren;
}

QVariant ProgramListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }
    switch (section) {
    case NameColumn:
        return tr("Program");
    case ChannelColumn:
        return tr("MIDI channel");
    default:
        return {};
    }
}

int ProgramListModel::programId(int row) const
{
    if (row < 0 || row >= m_store.programs().size()) {
        return -1;
    }
    return m_store.programs().at(row).id;
}

// --- ControlsModel

ControlsModel::ControlsModel(ProgramStore &store,
                             QUndoStack &stack,
                             Controls controls,
                             QObject *parent)
    : QAbstractTableModel(parent)
    , m_store(store)
    , m_stack(stack)
    , m_controls(controls)
{
    // A change of the shown program: all its control cells (a Program is
    // replaced as a whole). A change of another program: nothing.
    connect(&store, &ProgramStore::programChanged, this, [this](int id) {
        if (id == m_id) {
            emit dataChanged(index(0, 0),
                             index(rowCount() - 1, columnCount() - 1),
                             {Qt::DisplayRole, Qt::EditRole});
        }
    });
    // The shown program is removed: show nothing.
    connect(&store, &ProgramStore::programAboutToBeRemoved, this, [this](int id) {
        if (id == m_id) {
            beginResetModel();
            m_id = -1;
            endResetModel();
        }
    });
    // A load may replace the shown program: show nothing, the window picks
    // a program again.
    connect(&store, &ProgramStore::programsAboutToBeReset, this, [this] {
        beginResetModel();
        m_id = -1;
    });
    connect(&store, &ProgramStore::programsReset, this, [this] { endResetModel(); });
}

void ControlsModel::setProgramId(int id)
{
    if (id == m_id) {
        return;
    }
    beginResetModel();
    m_id = m_store.find(id) ? id : -1;
    endResetModel();
}

int ControlsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || m_id == -1) {
        return 0;
    }
    // cppcheck-suppress duplicateValueTernary ; both counts are 8 today, but they are distinct
    return static_cast<int>(m_controls == Controls::Pads ? pad_count : knob_count);
}

int ControlsModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_controls == Controls::Pads ? 4 : 3;
}

const QStringList &ControlsModel::names(int column) const
{
    if (m_controls == Controls::Pads) {
        if (column == NoteColumn) {
            return midinames::notes();
        }
        if (column == ToggleColumn) {
            return midinames::toggleModes();
        }
    }
    return midinames::values();
}

std::optional<int> ControlsModel::value(const Program &program, const QModelIndex &index) const
{
    const auto r = static_cast<std::size_t>(index.row());
    const int column = index.column();
    if (m_controls == Controls::Pads) {
        const Pad &pad = program.pads.at(r);
        switch (column) {
        case NoteColumn:
            return pad.note;
        case PadProgramChangeColumn:
            return pad.programChange;
        case PadControlChangeColumn:
            return pad.controlChange;
        case ToggleColumn:
            return pad.toggle ? 1 : 0;
        default:
            return std::nullopt;
        }
    }
    const Knob &knob = program.knobs.at(r);
    switch (column) {
    case KnobControlChangeColumn:
        return knob.controlChange;
    case LowColumn:
        return knob.low;
    case HighColumn:
        return knob.high;
    default:
        return std::nullopt;
    }
}

Program ControlsModel::withValue(Program program, const QModelIndex &index, quint8 value) const
{
    const auto r = static_cast<std::size_t>(index.row());
    const int column = index.column();
    if (m_controls == Controls::Pads) {
        Pad &pad = program.pads.at(r);
        switch (column) {
        case NoteColumn:
            pad.note = value;
            break;
        case PadProgramChangeColumn:
            pad.programChange = value;
            break;
        case PadControlChangeColumn:
            pad.controlChange = value;
            break;
        case ToggleColumn:
            pad.toggle = value == 1;
            break;
        default:
            break;
        }
        return program;
    }
    Knob &knob = program.knobs.at(r);
    switch (column) {
    case KnobControlChangeColumn:
        knob.controlChange = value;
        break;
    case LowColumn:
        knob.low = value;
        break;
    case HighColumn:
        knob.high = value;
        break;
    default:
        break;
    }
    return program;
}

QVariant ControlsModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }
    // get(), not find(): data() runs for every cell, role and repaint, and
    // find() would copy the whole program each time.
    const StoredProgram *stored = m_store.get(m_id);
    if (stored == nullptr) {
        return {};
    }
    const std::optional<int> v = value(stored->program, index);
    if (!v) {
        return {};
    }
    switch (role) {
    case Qt::EditRole:
        return *v;
    case Qt::DisplayRole:
        return names(index.column()).at(*v);
    case MidiDataRole::MidiValues:
        return names(index.column());
    default:
        return {};
    }
}

bool ControlsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid) || role != Qt::EditRole) {
        return false;
    }
    const StoredProgram *stored = m_store.get(m_id);
    if (stored == nullptr) {
        return false;
    }
    // The largest value a field takes: the length of its list of names (0-1
    // for the toggle, 0-127 otherwise).
    const auto max = static_cast<int>(names(index.column()).size()) - 1;
    const std::optional<quint8> v = editorValue(value, max);
    if (!v) {
        return false;
    }
    const Program changed = withValue(stored->program, index, *v);
    return push(m_stack, makeSetProgramCommand(m_store, m_id, changed));
}

Qt::ItemFlags ControlsModel::flags(const QModelIndex &index) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return Qt::NoItemFlags;
    }
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemNeverHasChildren;
    // As in the SQLite model before item 4.7b: the toggle column is shown
    // but not enabled. Changing that is an item of its own.
    if (m_controls != Controls::Pads || index.column() != ToggleColumn) {
        flags |= Qt::ItemIsEnabled;
    }
    return flags;
}

QVariant ControlsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        // The vertical header numbers the controls 1 to 8, as on the device.
        return QAbstractTableModel::headerData(section, orientation, role);
    }
    if (m_controls == Controls::Pads) {
        switch (section) {
        case NoteColumn:
            return tr("Note");
        case PadProgramChangeColumn:
            return tr("Program change");
        case PadControlChangeColumn:
            return tr("Control change");
        case ToggleColumn:
            return tr("Mode");
        default:
            return {};
        }
    }
    switch (section) {
    case KnobControlChangeColumn:
        return tr("Control change");
    case LowColumn:
        return tr("Low");
    case HighColumn:
        return tr("High");
    default:
        return {};
    }
}

} // namespace lpd8
