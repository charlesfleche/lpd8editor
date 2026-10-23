#ifndef PROGRAMMODELS_H
#define PROGRAMMODELS_H

#include "program.h"

#include <QAbstractTableModel>

class QUndoStack;

namespace lpd8 {

class ProgramStore;

// The models of the window over a ProgramStore (doc/IMPROVEMENTS.md, item
// 4.5). They are driven only by the store's signals: whatever changes the
// store (an edit, undo, a MIDI dump, a load), the views follow. Their
// setData() pushes the undo commands of storecommands.h, so every edit made
// through a view can be undone.
//
// Both use MidiDataRole::MidiValues (enums.h) for the list of value names
// the editors offer, and Qt::EditRole for the value itself.

// The programs: one row per program, in the store's order. Column 0 is the
// name (what the program list shows), column 1 the MIDI channel.
class ProgramListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column { NameColumn = 0, ChannelColumn, ColumnCount };
    // The program id of a row, for the views (Qt::UserRole).
    static constexpr int IdRole = Qt::UserRole + 100;

    ProgramListModel(ProgramStore &store, QUndoStack &stack, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // The id of the program at 'row', or -1.
    int programId(int row) const;

private:
    ProgramStore &m_store;
    QUndoStack &m_stack;
};

// The pads, or the knobs, of one program: eight rows, one per control, and
// one column per field. Empty while no program is shown.
class ControlsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum class Controls { Pads, Knobs };
    enum PadColumn { NoteColumn = 0, PadProgramChangeColumn, PadControlChangeColumn, ToggleColumn };
    enum KnobColumn { KnobControlChangeColumn = 0, LowColumn, HighColumn };

    ControlsModel(ProgramStore &store,
                  QUndoStack &stack,
                  Controls controls,
                  QObject *parent = nullptr);

    // Shows the program 'id', or nothing for -1.
    void setProgramId(int id);
    int programId() const { return m_id; }

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    // The value of the field at 'index' (row: the control, column: the
    // field), and the program with that field set. They take the index, not
    // a row and a column, which could be swapped by mistake.
    std::optional<int> value(const Program &program, const QModelIndex &index) const;
    Program withValue(Program program, const QModelIndex &index, quint8 value) const;
    const QStringList &names(int column) const;

    ProgramStore &m_store;
    QUndoStack &m_stack;
    const Controls m_controls;
    int m_id{-1};
};

} // namespace lpd8

#endif // PROGRAMMODELS_H
