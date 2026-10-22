#include "db.h"

#include "lpd8_sysex.h"
#include "utils.h"

#include <array>
#include <exception>

#define GOTO_END_IF_FALSE(X)                                                                       \
    if (!X)                                                                                        \
        goto end;

namespace {

template<typename T>
bool error(const T &val)
{
    return val.lastError().isValid();
}

template<typename T>
QString errorText(const T &val)
{
    return errorText(val.lastError());
}

template<>
QString errorText(const QSqlError &val)
{
    Q_ASSERT(val.isValid());
    return val.text().trimmed();
}

// Rolls back the current transaction. Failing to do so is rare, but then
// the database state is unknown, which deserves a warning.
void rollback(QSqlDatabase &db)
{
    if (!db.rollback()) {
        qWarning() << "Failed to roll back a transaction:" << errorText(db);
    }
}

} // namespace

QList<int> programIds()
{
    QList<int> ret;

    QSqlQuery q;
    GOTO_END_IF_FALSE(q.prepare("select programId from programs order by programId"));
    GOTO_END_IF_FALSE(q.exec());

    while (q.next()) {
        ret += q.value(0).toInt();
    }

end:
    if (error(q)) {
        qWarning() << "Failed to retrieve a list of program ids:" << errorText(q);
    }
    return ret;
}

QString programName(int programId)
{
    QString ret;
    QSqlQuery q;
    GOTO_END_IF_FALSE(q.prepare("select name from programs where programId = ?"));
    q.addBindValue(programId);
    GOTO_END_IF_FALSE(q.exec());

    GOTO_END_IF_FALSE(q.first());
    ret = q.value(0).toString();

    {
        // Call next() outside Q_ASSERT: in a release build the whole assert
        // expression is removed, and a call inside it would not happen at
        // all. The block ends the variable's scope, because a goto may not
        // jump over an initialization into its scope.
        [[maybe_unused]] const bool hasMoreRows = q.next();
        Q_ASSERT(!hasMoreRows);
    }

end:
    if (error(q)) {
        qWarning() << "Failed to retrieve program" << programId << "name:" << errorText(q);
    }
    return ret;
}

QByteArray programSysex(int programId)
{
    QByteArray ret;

    bool success = false;

    sysex::addProgramHeader(ret, 1);

    int count = 0;

    QSqlQuery q;

    // channel

    GOTO_END_IF_FALSE(q.prepare("select channel from programs where programId = ?"));
    q.addBindValue(programId);
    GOTO_END_IF_FALSE(q.exec());

    GOTO_END_IF_FALSE(q.first());
    ret += q.value(0).value<char>();

    {
        // Call next() outside Q_ASSERT: in a release build the whole assert
        // expression is removed, and a call inside it would not happen at
        // all. The block ends the variable's scope, because a goto may not
        // jump over an initialization into its scope.
        [[maybe_unused]] const bool hasMoreRows = q.next();
        Q_ASSERT(!hasMoreRows);
    }

    // pads

    GOTO_END_IF_FALSE(q.prepare(
            "select note, pc, cc, toggle from pads where programId = ? order by controlId"));
    q.addBindValue(programId);
    GOTO_END_IF_FALSE(q.exec());

    count = 0;
    while (q.next()) {
        const QSqlRecord r = q.record();
        for (int i = 0; i < r.count(); ++i) {
            ret += r.value(i).value<char>();
        }
        ++count;
    }

    Q_ASSERT(count == sysex::padsCount());

    // knobs

    GOTO_END_IF_FALSE(
            q.prepare("select cc, low, high from knobs where programId = ? order by controlId"));
    q.addBindValue(programId);
    GOTO_END_IF_FALSE(q.exec());

    count = 0;
    while (q.next()) {
        const QSqlRecord r = q.record();
        for (int i = 0; i < r.count(); ++i) {
            ret += r.value(i).value<char>();
        }
        ++count;
    }

    Q_ASSERT(count == sysex::knobsCount());

    sysex::addFooter(ret);

    success = true;

end:
    if (!success) {
        qWarning() << "Failed to generate sysex for program" << programId;
        return {};
    }
    if (error(q)) {
        qWarning() << errorText(q);
        return {};
    }
    return ret;
}

namespace {

QString defaultProgramName(int programIds)
{
    return QString("P%1").arg(programIds);
}

bool updateProgramFromSysex(int programId, const QByteArray &sysex)
{
    // Message layout (doc/SYSEX.md): 8 header bytes, the MIDI channel, 4 bytes
    // per pad (note, PC, CC, toggle), 3 per knob (CC, low, high), end byte.
    constexpr qsizetype channel_offset = 8;
    constexpr qsizetype pads_offset = 9;
    constexpr qsizetype pad_size = 4;
    constexpr qsizetype knob_size = 3;
    const qsizetype knobs_offset = pads_offset + (pad_size * sysex::padsCount());

    // Check the message once, before reading anything. Every path that
    // stores a program goes through here (file import, device dump, undo),
    // so this is where foreign or truncated data is stopped. A valid message
    // has the right size, so every offset below is inside it.
    if (!sysex::isProgram(sysex)) {
        qWarning() << "Failed to update program from sysex: not an LPD8 program message" << sysex;
        return false;
    }

    // Values are bound as int. A byte above 127 is negative as a char, and
    // the CHECK constraints of the tables reject it.
    const auto byte = [&sysex](qsizetype offset) { return static_cast<int>(sysex.at(offset)); };

    QSqlQuery q;
    const auto failed = [&q, &sysex] {
        qWarning() << "Failed to update program from sysex:" << sysex << errorText(q);
        return false;
    };

    if (!q.prepare("update programs set channel = ? where programId = ?")) {
        return failed();
    }
    q.addBindValue(byte(channel_offset));
    q.addBindValue(programId);
    if (!q.exec()) {
        return failed();
    }

    if (!q.prepare("update pads set note = ?, pc = ?, cc = ?, toggle = ? "
                   "where programId = ? and controlId = ?")) {
        return failed();
    }
    for (int controlId = 1; controlId <= sysex::padsCount(); ++controlId) {
        const qsizetype offset = pads_offset + (pad_size * (controlId - 1));
        for (qsizetype i = 0; i < pad_size; ++i) {
            q.addBindValue(byte(offset + i));
        }
        q.addBindValue(programId);
        q.addBindValue(controlId);
        if (!q.exec()) {
            return failed();
        }
    }

    if (!q.prepare("update knobs set cc = ?, low = ?, high = ? "
                   "where programId = ? and controlId = ?")) {
        return failed();
    }
    for (int controlId = 1; controlId <= sysex::knobsCount(); ++controlId) {
        const qsizetype offset = knobs_offset + (knob_size * (controlId - 1));
        for (qsizetype i = 0; i < knob_size; ++i) {
            q.addBindValue(byte(offset + i));
        }
        q.addBindValue(programId);
        q.addBindValue(controlId);
        if (!q.exec()) {
            return failed();
        }
    }

    return true;
}

} // namespace

namespace {

// Runs 'work' in a database transaction: commits if it succeeds, rolls back
// if it fails, so a failure never leaves half of the changes behind.
template<typename Work>
bool inTransaction(const char *what, Work work)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) {
        qWarning() << "Failed to start a transaction to" << what << ":" << errorText(db);
        return false;
    }
    if (!work()) {
        rollback(db);
        return false;
    }
    if (!db.commit()) {
        qWarning() << "Failed to commit the transaction to" << what << ":" << errorText(db);
        rollback(db);
        return false;
    }
    return true;
}

// Inserts the program and returns its id, or -1. Runs inside the caller's
// transaction.
int insertProgram(const QString &name, const QByteArray &sysex, int programId)
{
    QSqlQuery q;

    int ret = -1;

    if (programId == -1) {
        GOTO_END_IF_FALSE(q.prepare("insert into programs default values;"));
        GOTO_END_IF_FALSE(q.exec());

        programId = q.lastInsertId().toInt();
    } else {
        GOTO_END_IF_FALSE(q.prepare("insert into programs (programId) values (?);"));
        q.addBindValue(programId);
        GOTO_END_IF_FALSE(q.exec());
    }

    GOTO_END_IF_FALSE(q.prepare("update programs set name = ? where programId = ?"));
    q.addBindValue(name.isNull() ? QVariant(defaultProgramName(programId)) : QVariant(name));
    q.addBindValue(programId);
    GOTO_END_IF_FALSE(q.exec());

    // XXX this is a lame check
    // Proper way would be to pass a QIODevice and register callbacks
    // ala SAX
    if (!sysex.isNull()) {
        GOTO_END_IF_FALSE(updateProgramFromSysex(programId, sysex));
    }

    ret = programId;

end:
    if (error(q)) {
        qWarning() << "Failed to create program: " << errorText(q);
        ret = -1;
    }
    return ret;
}

} // namespace

int createProgram(const QString &name, const QByteArray &sysex, int programId)
{
    int id = -1;
    inTransaction("create a program", [&] {
        id = insertProgram(name, sysex, programId);
        return id != -1;
    });
    return id;
}

bool fromSysex(int programId, const QByteArray &sysex)
{
    return inTransaction("update a program from SysEx",
                         [&] { return updateProgramFromSysex(programId, sysex); });
}

bool deleteProgram(int programId)
{
    QSqlQuery q;
    GOTO_END_IF_FALSE(q.prepare("delete from programs where programId = ?"));
    q.addBindValue(programId);
    GOTO_END_IF_FALSE(q.exec());

    Q_ASSERT(q.numRowsAffected() < 2);

    if (q.numRowsAffected() == 0) {
        qWarning() << "Cannot delete inexistant program" << programId;
        return false;
    }

end:
    if (error(q)) {
        qWarning() << "Failed to delete program: " << errorText(q);
        return false;
    }
    return true;
}

namespace {

// The schema, in creation order: the tables first, then the triggers that
// use them.
constexpr std::array<const char *, 5> schema_files{
        ":/create_table_programs.sql",
        ":/create_table_pads.sql",
        ":/create_table_knobs.sql",
        ":/create_trigger_add_program.sql",
        ":/create_trigger_delete_program.sql",
};

// Creates the schema in one transaction (SQLite can roll back CREATE
// statements), and returns the error of the statement that failed, or an
// invalid QSqlError on success. QSqlDatabase is a handle to a connection,
// so a copy is cheap and refers to the same connection.
QSqlError initialize(QSqlDatabase db)
{
    if (!db.transaction()) {
        return db.lastError();
    }
    // Run the files in order, stopping at the first one that fails. A loop
    // rather than std::ranges::find_if: the 'predicate' would run SQL, and
    // [alg.find] specifies find_if's result, not the order in which it
    // applies the predicate.
    QSqlQuery q(db);
    for (const char *file : schema_files) {
        // cppcheck-suppress useStlAlgorithm ; the loop body has side effects
        if (!q.exec(readTextFile(file))) {
            // A failing query records its error on itself, not on the
            // database: db.lastError() would say that all is well.
            QSqlError error = q.lastError();
            rollback(db);
            return error;
        }
    }
    if (!db.commit()) {
        QSqlError error = db.lastError();
        rollback(db);
        return error;
    }
    return {};
}

bool isInitialized(const QSqlDatabase &db)
{
    return db.tables().contains("programs");
}

} // namespace

QSqlError initDb(const QSqlDatabase &db)
{
    // The .sql files are resources of a static library, which must be
    // registered by hand. Q_INIT_RESOURCE declares qInitResources_sql() in
    // the current namespace, so it must not be used inside a namespace: in
    // the anonymous one above, the declaration would have internal linkage
    // and never match the definition generated by rcc.
    Q_INIT_RESOURCE(sql);

    QSqlDriver *driver = db.driver();

    Q_ASSERT(driver);
    Q_ASSERT(driver->hasFeature(QSqlDriver::Transactions));
    Q_ASSERT(driver->hasFeature(QSqlDriver::Unicode));
    Q_ASSERT(driver->hasFeature(QSqlDriver::PreparedQueries));
    Q_ASSERT(driver->hasFeature(QSqlDriver::PositionalPlaceholders));
    Q_ASSERT(driver->hasFeature(QSqlDriver::LastInsertId));
    Q_ASSERT(driver->hasFeature(QSqlDriver::EventNotifications));

    if (isInitialized(db)) {
        return {};
    }
    QSqlError error = initialize(db);
    if (error.isValid()) {
        qWarning() << "Failed to initialize db" << db.databaseName() << ":" << error.text();
    }
    return error;
}

std::expected<QSqlDatabase, QString> openDatabase(const QString &path,
                                                  QLatin1StringView connectionName)
{
    const auto tr = [](const char *text) { return QCoreApplication::translate("Database", text); };

    const QString folder = QFileInfo(path).absolutePath();
    if (!QDir().mkpath(folder)) {
        return std::unexpected(tr("Cannot create the folder %1.").arg(folder));
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(path);
    if (!db.open()) {
        return std::unexpected(
                tr("Cannot open the database %1:\n%2").arg(path, db.lastError().text()));
    }

    // initDb() throws if a schema file is missing from the resources, which
    // is a packaging bug, but still better reported than a crash.
    try {
        const QSqlError error = initDb(db);
        if (error.isValid()) {
            return std::unexpected(
                    tr("Cannot create the database %1:\n%2").arg(path, error.text()));
        }
    } catch (const std::exception &e) {
        return std::unexpected(
                tr("Cannot create the database %1:\n%2").arg(path, QString::fromUtf8(e.what())));
    }
    return db;
}
