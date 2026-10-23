#ifndef PROGRAMSTORE_H
#define PROGRAMSTORE_H

#include "program.h"

#include <QList>
#include <QObject>
#include <QString>

#include <expected>
#include <limits>
#include <optional>

namespace lpd8 {

// A program as the user keeps it: the device content, plus what only the
// application knows, a stable id and a name.
struct StoredProgram
{
    int id{-1};
    QString name;
    Program program;

    bool operator==(const StoredProgram &) const = default;
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
// Ids are unique and never handed out twice, not even across sessions: the
// file stores the next id to hand out. A deleted program's id stays free, so
// undo can restore the program under it, and anything that remembers an id
// outside the store (the last selected program, a migration's mapping) can
// never find another program under it after a restart.
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

    // Adds a program at the end and returns its id: a new one, or 'id' if
    // it is given (to restore a deleted program). Returns nothing if 'id' is
    // taken or out of range, if no id is left, or if 'name' or 'program' is
    // not acceptable.
    std::optional<int> add(const QString &name, const Program &program, int id = -1);
    bool remove(int id);
    // False if there is no such program, or 'program' (or 'name') is not
    // acceptable.
    bool setProgram(int id, const Program &program);
    bool setName(int id, const QString &name);

    // Replaces the programs with those of the file. A file that doesn't
    // exist is an empty store (the first start). On failure, the store is
    // unchanged, the message says what is wrong for the user, and save()
    // refuses to write until setAsideUnreadableFile(): the file may hold the
    // user's programs in a form this version can't read (a newer format, a
    // hand edit), and must not be replaced.
    std::expected<void, QString> load();
    // Writes the programs atomically (QSaveFile), creating the folder if
    // needed: on failure, the previous file is intact.
    std::expected<void, QString> save() const;

    // After a failed load(): the reason, for the user, or an empty string.
    const QString &loadError() const { return m_loadError; }
    // False after a failed load(), until setAsideUnreadableFile().
    bool isWritable() const { return m_loadError.isEmpty(); }
    // Renames the file that load() could not read to
    // '<file>.corrupt-<yyyyMMdd-hhmmss>', next to it, so the store can be
    // saved again, with the programs it holds now (a failed load left them
    // unchanged). It never deletes anything. Returns the new name. The UI
    // must tell the user and ask first (doc/IMPROVEMENTS.md, item 4.11).
    std::expected<QString, QString> setAsideUnreadableFile();

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
    int m_nextId{1};
    // Set when load() fails: the file must not be overwritten.
    QString m_loadError;
};

// The JSON format, version 1. Separate from the file I/O so it can be tested
// without files. It stores each program's fields, not a SysEx message: the
// file stays readable and editable by hand, and independent of the device's
// encoding.
//
// Reading is strict: a key this version doesn't know is an error, not
// ignored. Ignoring it would drop it silently at the next save, and a typo
// in a hand edit ("Toggle") would go unnoticed. A future version that adds
// keys raises the format number.
inline constexpr int store_format_version = 1;
// Far above any real file (a program takes about 2 KB): only a wrong file is
// this large.
inline constexpr qint64 max_store_file_size = qint64{8} * 1024 * 1024;

struct StoreContent
{
    QList<StoredProgram> programs;
    int nextId{1}; // the next id to hand out, always above every id in use
};

QByteArray toJson(const StoreContent &content);
std::expected<StoreContent, QString> fromJson(const QByteArray &json);

} // namespace lpd8

#endif // PROGRAMSTORE_H
