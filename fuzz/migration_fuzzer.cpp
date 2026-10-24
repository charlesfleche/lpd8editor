// The one-time migration from the old SQLite database
// (lpd8::migrateSqliteDatabase()). Versions 0.0.0 to 0.0.5 created their
// tables without CHECK constraints, so a column can hold anything: a value
// out of range, a 64-bit integer, a REAL, text, a BLOB, NULL, or rows can
// be missing. The input chooses those values (not the bytes of a database
// file, which would mostly fuzz SQLite itself) for a database with that old
// schema. Then:
//
// - the migration succeeds: the database is readable, whatever it holds;
// - every program is either migrated or reported as skipped;
// - the programs file it writes loads again;
// - a migrated program holds exactly the database's values, checked
//   against an oracle that reads them with SQL's typeof() rather than
//   through QVariant like the migration does.

#include "fuzzer.h"

#include "migration.h"
#include "programstore.h"

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>
#include <QVariant>

#include <array>

namespace {

constexpr int max_programs = 3;
constexpr const char *connection_name = "fuzz";

// Reads the input a byte at a time; 0 when it runs out.
class Reader
{
public:
    explicit Reader(QByteArrayView bytes)
        : m_bytes(bytes)
    { }

    quint8 byte() { return m_at < m_bytes.size() ? static_cast<quint8>(m_bytes[m_at++]) : 0; }

    // A value of any SQLite type, mostly small integers.
    QVariant value()
    {
        const quint8 tag = byte();
        switch (tag % 10) {
        case 0: { // any 64-bit integer
            quint64 v = 0;
            for (int i = 0; i < 8; ++i) {
                v = (v << 8U) | byte();
            }
            return QVariant::fromValue(static_cast<qlonglong>(v));
        }
        case 1:
            return QVariant::fromValue(byte() / 2.0); // a REAL, maybe whole
        case 2:
            return QString::number(byte()); // text, that INTEGER affinity converts
        case 3:
            return QByteArray(1, static_cast<char>(byte())); // a BLOB
        case 4:
            return {}; // NULL
        case 5:
            return QVariant::fromValue(-static_cast<qlonglong>(byte())); // negative
        default:
            return QVariant::fromValue(static_cast<qlonglong>(byte())); // 0-255
        }
    }

    // A name: UTF-8, or UTF-16 code units that may be lone surrogates.
    QVariant name()
    {
        const quint8 tag = byte();
        const int length = byte() % 8;
        if (tag % 4 == 0) {
            QString utf16;
            for (int i = 0; i < length; ++i) {
                const unsigned high = byte();
                const unsigned low = byte();
                utf16 += QChar(static_cast<char16_t>((high << 8U) | low));
            }
            return utf16;
        }
        if (tag % 4 == 1) {
            return value();
        }
        QByteArray utf8;
        for (int i = 0; i < length; ++i) {
            utf8 += static_cast<char>(byte());
        }
        return QString::fromUtf8(utf8);
    }

private:
    QByteArrayView m_bytes;
    qsizetype m_at{0};
};

bool exec(QSqlDatabase &db, const QString &statement, const QVariantList &values = {})
{
    QSqlQuery q(db);
    if (!q.prepare(statement)) {
        return false;
    }
    for (const QVariant &v : values) {
        q.addBindValue(v);
    }
    return q.exec();
}

// Builds the old database at 'path' from the input.
void build(const QString &path, Reader &in)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connection_name);
    db.setDatabaseName(path);
    check(db.open());
    // The schema of v0.0.0 to v0.0.5 (as in tests/migration_test.cpp).
    check(exec(db,
               "create table programs(programId integer primary key, name varchar, "
               "channel integer)"));
    check(exec(db,
               "create table pads(programId integer NOT NULL, controlId integer NOT NULL, "
               "note integer NOT NULL, pc integer NOT NULL, cc integer NOT NULL, "
               "toggle integer NOT NULL, PRIMARY KEY (programId, controlId))"));
    check(exec(db,
               "create table knobs(programId integer NOT NULL, controlId integer NOT NULL, "
               "cc integer NOT NULL, low integer NOT NULL, high integer NOT NULL, "
               "PRIMARY KEY (programId, controlId))"));

    const int programs = 1 + (in.byte() % max_programs);
    for (int p = 0; p < programs; ++p) {
        // Most ids are small integers, so that programs have rows to find.
        const quint8 idTag = in.byte();
        const QVariant id =
                idTag % 4 == 0 ? in.value() : QVariant::fromValue(static_cast<qlonglong>(p + 1));
        // Inserts may fail (a duplicate id, a REAL id): that is fine, the
        // database holds what SQLite accepted.
        exec(db, "insert into programs values (?, ?, ?)", {id, in.name(), in.value()});
        // Usually 8 rows of each, sometimes fewer or more.
        const quint8 rowsTag = in.byte();
        const int rows = rowsTag < 200 ? 8 : rowsTag % 10;
        for (int r = 1; r <= rows; ++r) {
            exec(db,
                 "insert into pads values (?, ?, ?, ?, ?, ?)",
                 {id, r, in.value(), in.value(), in.value(), in.value()});
            exec(db,
                 "insert into knobs values (?, ?, ?, ?, ?)",
                 {id, r, in.value(), in.value(), in.value()});
        }
    }
    db.close();
}

// The oracle: the values of program 'id' in the database, read with SQL, if
// they make a program the migration must keep. Nothing if a value is not an
// INTEGER in its range, or rows are missing.
std::optional<lpd8::Program> oracle(QSqlDatabase &db, int id)
{
    // A field of the query: its column, and the largest value it takes.
    // Columns come in pairs: the value, then typeof(value).
    struct Field
    {
        int column;
        quint8 max;
    };
    const auto integerIn = [](const QSqlQuery &q, Field field) -> std::optional<quint8> {
        if (q.value(field.column + 1).toString() != QLatin1String("integer")) {
            return std::nullopt;
        }
        const qlonglong v = q.value(field.column).toLongLong();
        if (v < 0 || v > field.max) {
            return std::nullopt;
        }
        return static_cast<quint8>(v);
    };

    constexpr auto value_max = static_cast<quint8>(lpd8::midi_value_max);
    lpd8::Program program;
    QSqlQuery q(db);
    check(q.prepare("select channel, typeof(channel) from programs where programId = ?"));
    q.addBindValue(id);
    check(q.exec());
    if (!q.next()) {
        return std::nullopt;
    }
    const auto channel =
            integerIn(q, {.column = 0, .max = static_cast<quint8>(lpd8::midi_channel_max)});
    if (!channel) {
        return std::nullopt;
    }
    program.channel = *channel;

    check(q.prepare("select note, typeof(note), pc, typeof(pc), cc, typeof(cc), toggle, "
                    "typeof(toggle) from pads where programId = ? order by controlId limit 8"));
    q.addBindValue(id);
    check(q.exec());
    for (lpd8::Pad &pad : program.pads) {
        if (!q.next()) {
            return std::nullopt;
        }
        const auto note = integerIn(q, {.column = 0, .max = value_max});
        const auto pc = integerIn(q, {.column = 2, .max = value_max});
        const auto cc = integerIn(q, {.column = 4, .max = value_max});
        const auto toggle = integerIn(q, {.column = 6, .max = 1});
        if (!note || !pc || !cc || !toggle) {
            return std::nullopt;
        }
        pad = {.note = *note, .programChange = *pc, .controlChange = *cc, .toggle = *toggle == 1};
    }

    check(q.prepare("select cc, typeof(cc), low, typeof(low), high, typeof(high) from knobs "
                    "where programId = ? order by controlId limit 8"));
    q.addBindValue(id);
    check(q.exec());
    for (lpd8::Knob &knob : program.knobs) {
        if (!q.next()) {
            return std::nullopt;
        }
        const auto cc = integerIn(q, {.column = 0, .max = value_max});
        const auto low = integerIn(q, {.column = 2, .max = value_max});
        const auto high = integerIn(q, {.column = 4, .max = value_max});
        if (!cc || !low || !high) {
            return std::nullopt;
        }
        knob = {.controlChange = *cc, .low = *low, .high = *high};
    }
    return program;
}

} // namespace

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    // QtSql finds its SQLite driver through the application. It is never
    // destroyed: libFuzzer exits without returning, and destroying it with
    // the other statics, after Qt's own, crashes. Reachable through 'app',
    // so LeakSanitizer doesn't report it.
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    static const QCoreApplication *const app = new QCoreApplication(*argc, *argv);
    Q_UNUSED(app);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size)
{
    const QTemporaryDir dir;
    check(dir.isValid());
    const QString sqlitePath = dir.filePath("db.sqlite3");
    const QString jsonPath = dir.filePath("programs.json");

    Reader in(input(data, size));
    {
        build(sqlitePath, in);
    }
    QSqlDatabase::removeDatabase(connection_name);

    const auto migrated = lpd8::migrateSqliteDatabase(sqlitePath, jsonPath);
    check(migrated.has_value() && migrated->has_value());
    const lpd8::MigrationReport &report = migrated.value().value();

    lpd8::ProgramStore store(jsonPath);
    check(store.load().has_value());
    check(store.programs().size() == report.migrated);

    // Every program row is migrated or reported, and the migrated ones hold
    // what the oracle reads, under their old id.
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connection_name);
        db.setDatabaseName(report.oldFileNowAt);
        check(db.open());
        QSqlQuery q(db);
        check(q.exec("select programId, typeof(programId) from programs"));
        int rows = 0;
        while (q.next()) {
            ++rows;
            const bool usableId = q.value(1).toString() == QLatin1String("integer")
                    && q.value(0).toLongLong() >= 1
                    && q.value(0).toLongLong() <= lpd8::max_program_id;
            const auto expected =
                    usableId ? oracle(db, static_cast<int>(q.value(0).toLongLong())) : std::nullopt;
            const lpd8::StoredProgram *stored =
                    usableId ? store.get(static_cast<int>(q.value(0).toLongLong())) : nullptr;
            if (stored != nullptr) {
                check(expected.has_value());
                check(stored->program == expected.value());
            } else if (expected) {
                // Only the name can make a program with good values
                // unstorable (not valid UTF-16).
                check(!report.skipped.isEmpty());
            }
        }
        check(rows == report.migrated + report.skipped.size());
        db.close();
    }
    QSqlDatabase::removeDatabase(connection_name);
    return 0;
}
