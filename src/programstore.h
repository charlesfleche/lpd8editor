#ifndef PROGRAMSTORE_H
#define PROGRAMSTORE_H

#include "program.h"

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>

#include <expected>
#include <limits>
#include <optional>

namespace lpd8 {

// A program as the user keeps it: the device content, plus a name, which
// only the application knows. This is what the programs file holds.
struct NamedProgram
{
    QString name;
    Program program;

    bool operator==(const NamedProgram &) const = default;
};

// A program in the store: the same, plus the id the store gave it. The id
// only lives in memory: it is not in the file.
struct StoredProgram
{
    int id{-1};
    QString name;
    Program program;

    bool operator==(const StoredProgram &) const = default;
};

// Why the programs file couldn't be loaded. The kind decides what the user
// can be offered (doc/IMPROVEMENTS.md, item 4.11b): only a damaged file may be
// set aside.
struct LoadError
{
    enum class Kind {
        // The file can't be read: permissions (on it or on a folder above
        // it), an I/O error, or not a regular file (a directory). It may be
        // fine.
        CannotRead,
        // Read, but not a programs file this version accepts: not JSON, too
        // large, unknown keys, values out of range.
        Damaged,
        // Written by a newer lpd8editor: never read half of it, nor replace
        // it.
        NewerFormat,
    };

    // The default is the strictest kind: an error built without a kind
    // must never offer to set a file aside.
    Kind kind{Kind::CannotRead};
    QString message; // for the user
};

// Ids go from 1 to max_program_id. The cap leaves room for 'id + 1' (the
// next id) without a signed overflow.
inline constexpr int max_program_id = std::numeric_limits<int>::max() - 1;

// The user's programs, in memory, and the one JSON file they live in
// (doc/ARCHITECTURE.md, "Storage"). The file is fixed at construction, so a
// file that could not be read can never be overwritten by mistake.
//
// programs() is in the order programs were added, which is also the order
// a model shows them: the signals give each program's row in that list.
//
// Every change emits a signal *before* it (so a QAbstractItemModel can call
// beginInsertRows/beginRemoveRows/beginResetModel) and one *after* it. A
// change that changes nothing emits nothing and returns true. A rename and a
// program change have separate signals: a rename touches one column of a
// view, a program change the others.
//
// Ids name a program while the application runs: an undo command or a
// pending MIDI request holds an id, which stays right when rows move. The
// store hands them out, counting up, and never twice in its lifetime: a
// deleted program's id stays free, so undo can restore the program under it,
// and nothing that remembers an id can find another program under it. Ids
// are not saved: nothing outlives the application that could remember one.
// load() gives every program of the file a new id, in the file's order, so
// an id from before a load names nothing after it.
//
// Names are labels: duplicates are allowed. A name must be valid UTF-16
// (QString::isValidUtf16(), no lone surrogate), because JSON is UTF-8 and an
// invalid name would silently come back different after a save and load.
class ProgramStore : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ProgramStore)

public:
    explicit ProgramStore(QString path, QObject *parent = nullptr);
    ~ProgramStore() override;

    const QString &path() const { return m_path; }
    const QList<StoredProgram> &programs() const { return m_programs; }
    std::optional<StoredProgram> find(int id) const;
    // The same without a copy, for code that reads often (a model's data()):
    // nullptr if there is no such program. The pointer is valid until the
    // store changes.
    const StoredProgram *get(int id) const;
    // The row of program 'id' in programs(), if there is one.
    std::optional<int> rowOf(int id) const;
    // Adds a program at the end and returns its id: a new one, or 'id' if
    // it is given (to restore a deleted program). Returns nothing if 'id' is
    // taken or out of range, if no id is left, or if 'name' or 'program' is
    // not acceptable.
    std::optional<int> add(const QString &name, const Program &program, int id = -1);
    // The same, at 'row' (0 to programs().size()): undoing a removal puts the
    // program back where it was. Returns nothing if 'row' is out of range.
    std::optional<int> insert(int row, const QString &name, const Program &program, int id = -1);
    bool remove(int id);
    // False if there is no such program, or 'program' (or 'name') is not
    // acceptable.
    bool setProgram(int id, const Program &program);
    bool setName(int id, const QString &name);

    // Replaces the programs with those of the file, under new ids. A file
    // that doesn't exist is an empty store (the first start). On failure, the store is
    // unchanged, the message says what is wrong for the user, and save()
    // refuses to write until setAsideUnreadableFile(): the file may hold the
    // user's programs in a form this version can't read (a newer format, a
    // hand edit), and must not be replaced.
    std::expected<void, LoadError> load();
    // Writes the programs atomically (QSaveFile), creating the folder if
    // needed: on failure, the previous file is intact.
    std::expected<void, QString> save() const;

    // After a failed load(): why, or nothing.
    const std::optional<LoadError> &loadError() const { return m_loadError; }
    // False after a failed load(), until setAsideUnreadableFile().
    bool isWritable() const { return !m_loadError.has_value(); }
    // Renames the file that load() found Damaged to
    // '<file>.corrupt-<yyyyMMdd-hhmmss>', next to it, so the store can be
    // saved again, with the programs it holds now (a failed load left them
    // unchanged). It never deletes anything. Returns the new name. The UI
    // must tell the user and ask first (doc/IMPROVEMENTS.md, item 4.11b).
    // It refuses after any other load error: a directory, a FIFO or a file
    // it can't read may be fine, and a newer format certainly is.
    // 'now' names the file: a test can pick it, to make the rename fail.
    std::expected<QString, QString>
    setAsideUnreadableFile(const QDateTime &now = QDateTime::currentDateTime());

signals:
    void programAboutToBeAdded(int id, int row);
    void programAdded(int id, int row);
    void programAboutToBeRemoved(int id, int row);
    void programRemoved(int id, int row);
    void programChanged(int id, int row);
    void nameChanged(int id, int row);
    // Around load(): everything may change.
    void programsAboutToBeReset();
    void programsReset();

private:
    qsizetype indexOf(int id) const;

    QString m_path;
    QList<StoredProgram> m_programs;
    int m_nextId{1}; // the next id to hand out, above every id handed out
    // Set when load() fails: the file must not be overwritten.
    std::optional<LoadError> m_loadError;
};

// The JSON format, version 1. Separate from the file I/O so it can be tested
// without files. It stores each program's fields, not a SysEx message: the
// file stays readable and editable by hand, and independent of the device's
// encoding. A program has no id there: the file is the list of programs, in
// order, so a hand edit can copy, move or delete one with nothing to keep
// in step.
//
// Reading is strict: a key this version doesn't know is an error, not
// ignored. Ignoring it would drop it silently at the next save, and a typo
// in a hand edit ("Toggle") would go unnoticed. A future version that adds
// keys raises the format number.
inline constexpr int store_format_version = 1;
// Far above any real file (a program takes about 2 KB): only a wrong file is
// this large.
inline constexpr qint64 max_store_file_size = qint64{8} * 1024 * 1024;

QByteArray toJson(const QList<NamedProgram> &programs);
// The error is Damaged, or NewerFormat. Its message has no file name, and
// names a program by its position in the file, from 1.
std::expected<QList<NamedProgram>, LoadError> fromJson(const QByteArray &json);

} // namespace lpd8

#endif // PROGRAMSTORE_H
