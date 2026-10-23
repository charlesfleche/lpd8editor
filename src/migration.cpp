#include "migration.h"

#include "exactinteger.h"
#include "programstore.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <array>

namespace lpd8 {

namespace {

QString tr(const char *text, int n = -1)
{
    return QCoreApplication::translate("Migration", text, nullptr, n);
}

// The connection name used only by the migration, so it never touches the
// default connection, which other code may use.
constexpr const char *connection_name = "lpd8editor-migration";

using Error = std::unexpected<QString>;

// How a stored value is shown in a report: empty, a number, or quoted text.
QString shown(const QVariant &value)
{
    if (value.isNull()) {
        return tr("empty");
    }
    if (value.typeId() == QMetaType::QString) {
        return QStringLiteral("'%1'").arg(value.toString());
    }
    return value.toString();
}

// An integer field from 0 to 'max'. Old databases have no CHECK constraints,
// so a column can hold anything: exactInteger() takes only an INTEGER in range
// (never a cast: 300 must not become 44, nor 4294967336 become 40).
std::expected<int, QString> integerField(const QVariant &value, const char *name, int max)
{
    const auto v = exactInteger(value, max);
    if (!v) {
        return Error(tr("%1 is %2, not an integer from 0 to %3")
                             .arg(QString::fromLatin1(name), shown(value), QString::number(max)));
    }
    return *v;
}

std::expected<int, QString> field(const QSqlQuery &q, int column, const char *name, int max)
{
    return integerField(q.value(column), name, max);
}

std::expected<quint8, QString> midiField(const QSqlQuery &q, int column, const char *name)
{
    return field(q, column, name, midi_value_max).transform([](int v) {
        return static_cast<quint8>(v);
    });
}

// The 8 pads of 'programId', in control order.
std::expected<std::array<Pad, pad_count>, QString> readPads(QSqlDatabase &db, int programId)
{
    QSqlQuery q(db);
    if (!q.prepare(
                "select note, pc, cc, toggle from pads where programId = ? order by controlId")) {
        return Error(q.lastError().text());
    }
    q.addBindValue(programId);
    if (!q.exec()) {
        return Error(q.lastError().text());
    }
    std::array<Pad, pad_count> pads{};
    int number = 0; // 1-8, as on the device
    for (Pad &pad : pads) {
        ++number;
        if (!q.next()) {
            return Error(tr("pad %1 is missing").arg(number));
        }
        const auto note = midiField(q, 0, "note");
        const auto programChange = midiField(q, 1, "program change");
        const auto controlChange = midiField(q, 2, "control change");
        const auto toggle = field(q, 3, "toggle", 1);
        for (const auto *error : {&note, &programChange, &controlChange}) {
            if (!*error) {
                return Error(tr("pad %1: %2").arg(QString::number(number), error->error()));
            }
        }
        if (!toggle) {
            return Error(tr("pad %1: %2").arg(QString::number(number), toggle.error()));
        }
        pad = Pad{.note = *note,
                  .programChange = *programChange,
                  .controlChange = *controlChange,
                  .toggle = *toggle == 1};
    }
    return pads;
}

// The 8 knobs of 'programId', in control order.
std::expected<std::array<Knob, knob_count>, QString> readKnobs(QSqlDatabase &db, int programId)
{
    QSqlQuery q(db);
    if (!q.prepare("select cc, low, high from knobs where programId = ? order by controlId")) {
        return Error(q.lastError().text());
    }
    q.addBindValue(programId);
    if (!q.exec()) {
        return Error(q.lastError().text());
    }
    std::array<Knob, knob_count> knobs{};
    int number = 0;
    for (Knob &knob : knobs) {
        ++number;
        if (!q.next()) {
            return Error(tr("knob %1 is missing").arg(number));
        }
        const auto controlChange = midiField(q, 0, "control change");
        const auto low = midiField(q, 1, "low");
        const auto high = midiField(q, 2, "high");
        for (const auto *error : {&controlChange, &low, &high}) {
            if (!*error) {
                return Error(tr("knob %1: %2").arg(QString::number(number), error->error()));
            }
        }
        knob = Knob{.controlChange = *controlChange, .low = *low, .high = *high};
    }
    return knobs;
}

struct OldProgram
{
    QVariant id; // a 64-bit rowid: checked before use, like every value
    QString name;
    QVariant channel;
};

// Reads every program it can into 'store', and reports the others.
std::expected<MigrationReport, QString> readInto(QSqlDatabase &db, ProgramStore &store)
{
    MigrationReport report;

    // In the order the old program list showed them: by id. The old list
    // showed the table's rows in rowid order, which is the id here, and
    // never sorted them (checked: the old ProgramsModel, deleted in item
    // 4.7b, had no setSort() or sort()).
    // A NULL name, which v0.0.0 to v0.0.5 allowed, becomes an empty name: the
    // old list showed both the same way.
    QSqlQuery q(db);
    if (!q.exec("select programId, name, channel from programs order by programId")) {
        return Error(q.lastError().text());
    }
    QList<OldProgram> programs;
    while (q.next()) {
        programs.append(
                OldProgram{.id = q.value(0), .name = q.value(1).toString(), .channel = q.value(2)});
    }

    for (const OldProgram &old : programs) {
        const auto skip = [&report, &old](const QString &why) {
            report.skipped.append(tr("program %1 (%2): %3").arg(shown(old.id), old.name, why));
        };

        // Through exactInteger() too: an id above INT_MAX must not wrap onto
        // another program's id.
        const auto id = exactInteger(old.id, max_program_id);
        if (!id || *id < 1) {
            skip(tr("its id is not an integer from 1 to %1").arg(max_program_id));
            continue;
        }

        const auto channel = integerField(old.channel, "channel", midi_channel_max);
        if (!channel) {
            skip(channel.error());
            continue;
        }
        const auto pads = readPads(db, *id);
        if (!pads) {
            skip(pads.error());
            continue;
        }
        const auto knobs = readKnobs(db, *id);
        if (!knobs) {
            skip(knobs.error());
            continue;
        }

        const Program program{
                .channel = static_cast<quint8>(*channel), .pads = *pads, .knobs = *knobs};
        // The old id is kept: ids are positive and unique there too.
        // ProgramStore refuses what it can't store (an id out of its range,
        // a name that isn't valid UTF-16): report that as well.
        if (!store.add(old.name, program, *id)) {
            skip(tr("it can't be stored (id or name not accepted)"));
            continue;
        }
        ++report.migrated;
    }
    return report;
}

// Saves describe(report) to 'path', atomically. Records the outcome in the
// report: describe() says where the text is, or why it isn't saved.
void saveReport(MigrationReport &report, const QString &path)
{
    report.reportFileAt = path;
    report.reportFileError.clear();
    QSaveFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)
        && f.write(describe(report).toUtf8() + '\n') >= 0 && f.commit()) {
        return;
    }
    report.reportFileAt.clear();
    report.reportFileError = tr("Cannot save this report to %1: %2")
                                     .arg(QDir::toNativeSeparators(path), f.errorString());
}

} // namespace

QString describe(const MigrationReport &report)
{
    QStringList parts;
    if (report.migrated == 0 && report.skipped.isEmpty()) {
        parts.append(tr("The old database held no programs, so the new programs file starts "
                        "empty."));
    } else {
        parts.append(tr("Your programs were moved from the old database to a new file. %n "
                        "program(s) moved.",
                        report.migrated));
    }
    if (!report.skipped.isEmpty()) {
        parts.append(tr("These programs could not be moved, and are still in the old "
                        "database:\n%1")
                             .arg(report.skipped.join(QLatin1Char('\n'))));
    }
    if (!report.oldFileNowAt.isEmpty()) {
        parts.append(tr("The old database is kept as a backup, at %1.")
                             .arg(QDir::toNativeSeparators(report.oldFileNowAt)));
    }
    if (!report.renameError.isEmpty()) {
        // It is not read again: the new file exists, so the migration is
        // done.
        parts.append(tr("%1 The old database is kept where it was, and won't be read again.")
                             .arg(report.renameError));
    }
    if (!report.reportFileAt.isEmpty()) {
        parts.append(tr("This report is saved in %1.")
                             .arg(QDir::toNativeSeparators(report.reportFileAt)));
    }
    if (!report.reportFileError.isEmpty()) {
        parts.append(report.reportFileError);
    }
    return parts.join(QStringLiteral("\n\n"));
}

std::expected<std::optional<MigrationReport>, QString>
migrateSqliteDatabase(const QString &sqlitePath, const QString &jsonPath)
{
    // Nothing to do, or already done: touch nothing.
    if (QFile::exists(jsonPath) || !QFile::exists(sqlitePath)) {
        return std::nullopt;
    }

    ProgramStore store(jsonPath);
    std::expected<MigrationReport, QString> report;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connection_name);
        db.setDatabaseName(sqlitePath);
        // Read-only: the migration must never change the old database.
        db.setConnectOptions("QSQLITE_OPEN_READONLY");
        if (!db.open()) {
            report = Error(db.lastError().text());
        } else {
            report = readInto(db, store);
        }
        db.close();
    }
    // Every QSqlDatabase object of the connection is gone: it can be removed.
    QSqlDatabase::removeDatabase(connection_name);
    if (!report) {
        return Error(tr("Cannot read the old programs database %1: %2")
                             .arg(QDir::toNativeSeparators(sqlitePath), report.error()));
    }

    // First the new file, atomically...
    if (const auto saved = store.save(); !saved) {
        return Error(saved.error());
    }

    // ...then, and only then, move the old one aside. If this fails, the
    // migration still happened: the JSON file exists, so it won't run again.
    // A name whose backup or report is left from an earlier migration is
    // taken: neither is ever replaced.
    QString aside = sqlitePath + QStringLiteral(".migrated");
    if (QFile::exists(aside) || QFile::exists(aside + QStringLiteral(".txt"))) {
        aside += QStringLiteral("-")
                + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss"));
    }
    // The report is saved first, saying where the backup will be: if the
    // program stops right after the rename, the list of skipped programs is
    // already next to them.
    const QString reportPath = aside + QStringLiteral(".txt");
    report->oldFileNowAt = aside;
    saveReport(*report, reportPath);
    if (!QFile::rename(sqlitePath, aside)) {
        report->oldFileNowAt.clear();
        report->renameError =
                tr("Cannot rename %1 to %2.")
                        .arg(QDir::toNativeSeparators(sqlitePath), QDir::toNativeSeparators(aside));
        saveReport(*report, reportPath);
    }
    return *report;
}

} // namespace lpd8
