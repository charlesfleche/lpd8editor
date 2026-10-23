#include "programsdocument.h"

#include <utility>

namespace lpd8 {

ProgramsDocument::ProgramsDocument(QString path, QObject *parent)
    : QObject(parent)
    , m_store(std::move(path))
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(0);
    connect(&m_saveTimer, &QTimer::timeout, this, [this] { save(false); });

    // Save on the store's own change signals, not on the undo stack's: then
    // a change made any way is saved, not only one that goes through the
    // stack. Undo and redo change the store too, and an obsolete command,
    // which changes nothing, writes nothing.
    connect(&m_store, &ProgramStore::programAdded, this, &ProgramsDocument::scheduleSave);
    connect(&m_store, &ProgramStore::programRemoved, this, &ProgramsDocument::scheduleSave);
    connect(&m_store, &ProgramStore::programChanged, this, &ProgramsDocument::scheduleSave);
    connect(&m_store, &ProgramStore::nameChanged, this, &ProgramsDocument::scheduleSave);

    // load() replaces the content: a pending save of the old content is
    // dropped, and the load itself writes nothing (only the reset signals
    // come, which don't save). Commands hold ids, which may name other
    // programs in the new content, so the stack is cleared; that changes no
    // program, so it doesn't save either.
    // After a load, the store holds what the file holds: nothing is left
    // to save.
    connect(&m_store, &ProgramStore::programsAboutToBeReset, this, [this] {
        m_saveTimer.stop();
        m_dirty = false;
        m_undoStack.clear();
    });
}

ProgramsDocument::~ProgramsDocument()
{
    // The members are destroyed after this body, in the reverse order of
    // their declaration: the timer first. Disconnect while everything is
    // alive, so no signal from a member being destroyed (the stack deleting
    // its commands, the store) reaches a member already gone.
    m_store.disconnect(this);
    m_saveTimer.stop();
}

void ProgramsDocument::setSaveFailureHandler(SaveFailureHandler handler)
{
    m_onSaveFailure = std::move(handler);
}

void ProgramsDocument::scheduleSave()
{
    m_dirty = true;
    if (!m_saveTimer.isActive()) {
        m_saveTimer.start();
    }
}

bool ProgramsDocument::saveNow()
{
    m_saveTimer.stop();
    // Closing the window and quitting both call this: the second call has
    // nothing new to save, or to tell, so it doesn't write or report again.
    if (!m_dirty) {
        return m_lastSaveSucceeded;
    }
    return save(true);
}

bool ProgramsDocument::save(bool reportEveryFailure)
{
    const auto result = m_store.save();
    m_lastSaveSucceeded = result.has_value();
    if (result) {
        m_dirty = false;
        m_failureReported = false;
    } else if ((reportEveryFailure || !m_failureReported) && m_onSaveFailure) {
        m_failureReported = true;
        m_onSaveFailure(result.error());
        // A failure saveNow() reported is settled: the user knows. One the
        // timer reported once leaves the changes to save, so the next
        // saveNow() (at quit) tries again: the last try.
        if (reportEveryFailure) {
            m_dirty = false;
        }
    }
    emit saved(result.has_value());
    return result.has_value();
}

} // namespace lpd8
