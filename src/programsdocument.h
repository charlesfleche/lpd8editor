#ifndef PROGRAMSDOCUMENT_H
#define PROGRAMSDOCUMENT_H

#include "programstore.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QUndoStack>

#include <functional>

namespace lpd8 {

// The user's programs as the window edits them: the store, its undo stack,
// and saving (doc/IMPROVEMENTS.md, item 4.7a).
//
// - The undo stack is declared after the store, so it is destroyed first: its
//   commands refer to the store.
// - The stack is cleared when the store is reset (load()): commands hold
//   program ids, which may name other programs in the new content.
// - Every change of the store saves it, whoever makes it (an edit through
//   the stack, undo, redo, or code that changes the store directly),
//   coalesced: all changes made in one turn of the event loop make one
//   write. Loading never writes, and neither does clearing the stack, which
//   changes no program.
// - A save that fails is reported once, through the failure handler, until a
//   save succeeds again: not one message per edit. saveNow(), the explicit
//   save when the window closes and the application quits, reports the
//   failure of every save it makes, since it is the last chance to tell the
//   user. The handler's owner shows the message; the
//   document never shows anything itself.
// - saveNow() only writes if something changed since the last save, and
//   since the last failure it reported: closing the window, then quitting,
//   call it twice, and a failing disk is reported once. A failure of a
//   save made by the timer leaves the changes to save, so saveNow() tries
//   again.
// - Destroying the document with a save pending does NOT save: a destructor
//   can't report a failure, and a refused startup must never write. Quitting
//   saves with saveNow().
class ProgramsDocument : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ProgramsDocument)

public:
    using SaveFailureHandler = std::function<void(const QString &error)>;

    explicit ProgramsDocument(QString path, QObject *parent = nullptr);
    ~ProgramsDocument() override;

    ProgramStore &store() { return m_store; }
    const ProgramStore &store() const { return m_store; }
    QUndoStack &undoStack() { return m_undoStack; }

    void setSaveFailureHandler(SaveFailureHandler handler);

    // Saves now, cancelling a pending save, and reports a failure. With
    // nothing new to save (see the class comment), it does nothing and
    // returns how the last save went.
    bool saveNow();

signals:
    // After every save attempt, scheduled or not.
    void saved(bool succeeded);

private:
    void scheduleSave();
    bool save(bool reportEveryFailure);

    // Order matters: see the class comment.
    ProgramStore m_store;
    QUndoStack m_undoStack;
    QTimer m_saveTimer;
    SaveFailureHandler m_onSaveFailure;
    bool m_failureReported{false};
    // Changed since the last save that succeeded, or whose failure
    // saveNow() reported.
    bool m_dirty{false};
    bool m_lastSaveSucceeded{true};
};

} // namespace lpd8

#endif // PROGRAMSDOCUMENT_H
