#include "deviceprofile.h"
#include "programstore.h"

#include <QFile>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <sys/stat.h>
#include <unistd.h>

using namespace lpd8;

namespace {

Program someProgram(quint8 channel = 9)
{
    Program p = profile().defaultProgram;
    p.channel = channel;
    return p;
}

QByteArray readAll(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

void writeAll(const QString &path, const QByteArray &bytes)
{
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(bytes);
}

bool hasProgram(const ProgramStore &store, int id)
{
    return store.find(id).has_value();
}

} // namespace

class TestProgramStore : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void test_addAndFind();
    void test_addRefuses();
    void test_idsRunOutWithoutOverflow();
    void test_remove();
    void test_insertAtRow();
    void test_signalsComeBeforeAndAfterTheChange();
    void test_setProgramAndNameSignalSeparately();
    void test_noOpSetsEmitNothing();
    void test_idsAreNotReusedInASession();
    void test_loadGivesNewIds();
    void test_theFileHasNoIds();
    void test_saveAndLoadRoundTrip();
    void test_loadMissingFileIsEmpty();
    void test_loadRejects_data();
    void test_loadRejects();
    void test_failedLoadKeepsStoreAndProtectsFile();
    void test_unreadableFileStaysProtectedUntilReadable();
    void test_newerFormatIsNotOverwritten();
    void test_tooLargeFileIsRefused();
    void test_saveNeverWritesWhatLoadRefuses();
    void test_setAsideKeepsTheBytes();
    void test_aDirectoryCannotBeRead();
    void test_aFifoDoesNotBlock();
    void test_aFolderThatCannotBeEnteredIsNotAFirstStart();
    void test_setAsideRefusesATakenName();
    void test_saveCreatesTheFolder();
    void test_saveReportsFailure();

private:
    std::unique_ptr<QTemporaryDir> dir;
    QString path; // a programs file in 'dir', not created
};

void TestProgramStore::init()
{
    dir = std::make_unique<QTemporaryDir>();
    QVERIFY(dir->isValid());
    path = dir->filePath("programs.json");
}

void TestProgramStore::test_addAndFind()
{
    ProgramStore store(path);
    const QSignalSpy added(&store, &ProgramStore::programAdded);

    // Ids count up from 1.
    const int id = store.add(QStringLiteral("Drums"), someProgram()).value();
    QCOMPARE(id, 1);
    QCOMPARE(added.count(), 1);
    QCOMPARE(added.at(0).at(0).toInt(), id);
    QCOMPARE(added.at(0).at(1).toInt(), 0); // the row

    const StoredProgram found = store.find(id).value();
    QCOMPARE(found.name, QStringLiteral("Drums"));
    QCOMPARE(found.program, someProgram());
    QVERIFY(!hasProgram(store, id + 1));
    QCOMPARE(store.get(id)->name, QStringLiteral("Drums"));
    QCOMPARE(store.get(id + 1), nullptr);

    // Names are labels: duplicates are allowed.
    QVERIFY(store.add(QStringLiteral("Drums"), someProgram()).has_value());
    QCOMPARE(store.programs().size(), 2);
    QCOMPARE(added.at(1).at(1).toInt(), 1);
}

void TestProgramStore::test_addRefuses()
{
    ProgramStore store(path);
    const QSignalSpy added(&store, &ProgramStore::programAdded);
    const int id = store.add(QStringLiteral("A"), someProgram()).value();

    Program invalid = someProgram();
    invalid.channel = 16;
    QVERIFY(!store.add(QStringLiteral("B"), invalid).has_value());
    QVERIFY(!store.add(QStringLiteral("B"), someProgram(), id).has_value()); // id taken
    QVERIFY(!store.add(QStringLiteral("B"), someProgram(), 0).has_value()); // ids start at 1
    QVERIFY(!store.add(QStringLiteral("B"), someProgram(), max_program_id + 1).has_value());
    // A lone surrogate can't be written as UTF-8: it would come back as U+FFFD.
    const QString loneSurrogate(QChar(0xd800));
    QVERIFY(!store.add(loneSurrogate, someProgram()).has_value());
    QVERIFY(!store.setName(id, loneSurrogate));

    QCOMPARE(added.count(), 1);
    QCOMPARE(store.programs().size(), 1);
}

void TestProgramStore::test_idsRunOutWithoutOverflow()
{
    // The largest id can be used, and then no id is left: add() refuses
    // instead of computing max_program_id + 2. Run under UBSan, which reports
    // a signed overflow.
    ProgramStore store(path);
    QCOMPARE(store.add(QStringLiteral("Last"), someProgram(), max_program_id),
             std::optional<int>(max_program_id));
    QVERIFY(!store.add(QStringLiteral("None left"), someProgram()).has_value());

    // A load needs a new id for each program of the file: it fails, and the
    // store is unchanged. The file is fine, so it is not Damaged (never set
    // aside).
    QVERIFY(store.save().has_value());
    const auto reloaded = store.load();
    QVERIFY(!reloaded.has_value());
    QCOMPARE(reloaded.error().kind, LoadError::Kind::CannotRead);
    QVERIFY2(reloaded.error().message.contains("no program id is left"),
             qPrintable(reloaded.error().message));
    QVERIFY(hasProgram(store, max_program_id));

    // Ids are not in the file: another store reads it, and starts from 1.
    ProgramStore loaded(path);
    QVERIFY(loaded.load().has_value());
    QCOMPARE(loaded.programs().at(0).id, 1);
    QVERIFY(loaded.add(QStringLiteral("Room left"), someProgram()).has_value());
}

void TestProgramStore::test_remove()
{
    ProgramStore store(path);
    const int id = store.add(QStringLiteral("A"), someProgram()).value();
    const QSignalSpy removed(&store, &ProgramStore::programRemoved);

    QVERIFY(store.remove(id));
    QCOMPARE(removed.count(), 1);
    QCOMPARE(removed.at(0).at(0).toInt(), id);
    QVERIFY(!store.remove(id));
    QCOMPARE(removed.count(), 1);
}

void TestProgramStore::test_insertAtRow()
{
    ProgramStore store(path);
    const int a = store.add(QStringLiteral("A"), someProgram()).value();
    const int c = store.add(QStringLiteral("C"), someProgram()).value();
    const QSignalSpy aboutToAdd(&store, &ProgramStore::programAboutToBeAdded);
    const QSignalSpy added(&store, &ProgramStore::programAdded);

    const int b = store.insert(1, QStringLiteral("B"), someProgram()).value();
    QCOMPARE(store.rowOf(a), std::optional<int>(0));
    QCOMPARE(store.rowOf(b), std::optional<int>(1));
    QCOMPARE(store.rowOf(c), std::optional<int>(2));
    QCOMPARE(aboutToAdd.at(0).at(1).toInt(), 1); // the row, before...
    QCOMPARE(added.at(0).at(1).toInt(), 1); // ...and after

    QVERIFY(!store.insert(-1, QStringLiteral("X"), someProgram()).has_value());
    QVERIFY(!store.insert(4, QStringLiteral("X"), someProgram()).has_value());
    QVERIFY(store.insert(3, QStringLiteral("D"), someProgram()).has_value()); // at the end
    QVERIFY(!store.rowOf(42).has_value());
}

void TestProgramStore::test_signalsComeBeforeAndAfterTheChange()
{
    // What a QAbstractItemModel needs: 'about to' while the data is still as
    // before (beginInsertRows, beginRemoveRows, beginResetModel), the other
    // signal once it has changed, and the row in programs().
    ProgramStore store(path);
    store.add(QStringLiteral("A"), someProgram());
    const int b = store.add(QStringLiteral("B"), someProgram()).value();
    QStringList seen;
    const auto sizeAndB = [&] {
        return QStringLiteral("size %1, B %2")
                .arg(QString::number(store.programs().size()),
                     hasProgram(store, b) ? QStringLiteral("present") : QStringLiteral("absent"));
    };
    connect(&store, &ProgramStore::programAboutToBeAdded, this, [&](int, int row) {
        seen << QStringLiteral("about to add row %1: %2").arg(QString::number(row), sizeAndB());
    });
    connect(&store, &ProgramStore::programAdded, this, [&](int, int row) {
        seen << QStringLiteral("added row %1: %2").arg(QString::number(row), sizeAndB());
    });
    connect(&store, &ProgramStore::programAboutToBeRemoved, this, [&](int id, int row) {
        QCOMPARE(store.programs().at(row).id, id); // still there, at that row
        seen << QStringLiteral("about to remove row %1: %2").arg(QString::number(row), sizeAndB());
    });
    connect(&store, &ProgramStore::programRemoved, this, [&](int, int row) {
        seen << QStringLiteral("removed row %1: %2").arg(QString::number(row), sizeAndB());
    });
    connect(&store, &ProgramStore::programsAboutToBeReset, this, [&] {
        seen << QStringLiteral("about to reset: %1").arg(sizeAndB());
    });
    connect(&store, &ProgramStore::programsReset, this, [&] {
        seen << QStringLiteral("reset: %1").arg(sizeAndB());
    });

    store.add(QStringLiteral("C"), someProgram());
    QVERIFY(store.remove(b));
    QVERIFY(store.load().has_value()); // no file yet: an empty store

    const QStringList expected{
            "about to add row 2: size 2, B present",
            "added row 2: size 3, B present",
            "about to remove row 1: size 3, B present",
            "removed row 1: size 2, B absent",
            "about to reset: size 2, B absent",
            "reset: size 0, B absent",
    };
    QCOMPARE(seen, expected);
}

void TestProgramStore::test_setProgramAndNameSignalSeparately()
{
    ProgramStore store(path);
    store.add(QStringLiteral("First"), someProgram());
    const int id = store.add(QStringLiteral("A"), someProgram()).value();
    const QSignalSpy changed(&store, &ProgramStore::programChanged);
    const QSignalSpy renamed(&store, &ProgramStore::nameChanged);

    QVERIFY(store.setName(id, QStringLiteral("B")));
    QCOMPARE(renamed.count(), 1);
    QCOMPARE(renamed.at(0).at(1).toInt(), 1); // the row
    QCOMPARE(changed.count(), 0);

    QVERIFY(store.setProgram(id, someProgram(3)));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(changed.at(0).at(1).toInt(), 1);
    QCOMPARE(renamed.count(), 1);
    QCOMPARE(store.find(id).value().program.channel, quint8(3));

    Program invalid = someProgram();
    invalid.pads[0].note = 200;
    QVERIFY(!store.setProgram(id, invalid));
    QVERIFY(!store.setProgram(id + 1, someProgram()));
    QCOMPARE(changed.count(), 1);
}

void TestProgramStore::test_noOpSetsEmitNothing()
{
    ProgramStore store(path);
    const int id = store.add(QStringLiteral("A"), someProgram()).value();
    const QSignalSpy changed(&store, &ProgramStore::programChanged);
    const QSignalSpy renamed(&store, &ProgramStore::nameChanged);

    QVERIFY(store.setName(id, QStringLiteral("A")));
    QVERIFY(store.setProgram(id, someProgram()));
    QCOMPARE(changed.count(), 0);
    QCOMPARE(renamed.count(), 0);
}

void TestProgramStore::test_idsAreNotReusedInASession()
{
    ProgramStore store(path);
    store.add(QStringLiteral("A"), someProgram());
    const int last = store.add(QStringLiteral("B"), someProgram()).value();
    QVERIFY(store.remove(last));
    QVERIFY(store.add(QStringLiteral("C"), someProgram()).value() != last);
    // Undo can still restore the deleted program under its id.
    QCOMPARE(store.add(QStringLiteral("B"), someProgram(), last), std::optional<int>(last));
}

void TestProgramStore::test_loadGivesNewIds()
{
    // An id from before a load must not name a program after it: the file
    // may hold other programs by then. So load() never reuses an id.
    ProgramStore store(path);
    const int a = store.add(QStringLiteral("A"), someProgram()).value();
    const int b = store.add(QStringLiteral("B"), someProgram(3)).value();
    QVERIFY(store.remove(a));
    QVERIFY(store.save().has_value());

    QVERIFY(store.load().has_value());
    QCOMPARE(store.programs().size(), 1);
    const StoredProgram loaded = store.programs().at(0);
    QCOMPARE(loaded.name, QStringLiteral("B"));
    QCOMPARE(loaded.program, someProgram(3));
    QVERIFY(loaded.id > b);
    QVERIFY(!hasProgram(store, a));
    QVERIFY(!hasProgram(store, b));
    // And new programs come after the loaded ones.
    QVERIFY(store.add(QStringLiteral("C"), someProgram()).value() > loaded.id);

    // A store that starts with the file numbers its programs from 1, in the
    // file's order.
    ProgramStore fresh(path);
    QVERIFY(fresh.load().has_value());
    QCOMPARE(fresh.programs().size(), 1);
    QCOMPARE(fresh.programs().at(0).id, 1);
}

void TestProgramStore::test_theFileHasNoIds()
{
    // Nothing in the file must be kept in step by hand: the same program
    // twice (a block copied and pasted) is two programs.
    const NamedProgram drums{.name = "Drums", .program = someProgram()};
    const QByteArray json = toJson({drums, drums});
    QVERIFY(!json.contains("id"));
    QVERIFY(!json.contains("Id"));
    QCOMPARE(fromJson(json).value(), (QList<NamedProgram>{drums, drums}));

    writeAll(path, json);
    ProgramStore store(path);
    QVERIFY(store.load().has_value());
    QCOMPARE(store.programs().size(), 2);
    QVERIFY(store.programs().at(0).id != store.programs().at(1).id);
}

void TestProgramStore::test_saveAndLoadRoundTrip()
{
    ProgramStore store(path);
    Program edited = someProgram(15);
    edited.pads[2].toggle = true;
    edited.knobs[7] = Knob{.controlChange = 1, .low = 10, .high = 100};
    // Unicode names: accented, CJK, an emoji (outside the BMP: a surrogate
    // pair in UTF-16) and right-to-left text.
    for (const QString &name : {QStringLiteral("Café"),
                                QStringLiteral("日本語"),
                                QStringLiteral("Drums 🥁"),
                                QStringLiteral("طبول")}) {
        QVERIFY(store.add(name, edited).has_value());
    }
    QVERIFY(store.save().has_value());

    ProgramStore loaded(path);
    const QSignalSpy reset(&loaded, &ProgramStore::programsReset);
    QVERIFY(loaded.load().has_value());
    QCOMPARE(reset.count(), 1);
    // Names and programs, in order. The ids too: both stores counted from 1.
    QCOMPARE(loaded.programs(), store.programs());
}

void TestProgramStore::test_loadMissingFileIsEmpty()
{
    ProgramStore store(path);
    const QSignalSpy aboutToReset(&store, &ProgramStore::programsAboutToBeReset);
    const QSignalSpy reset(&store, &ProgramStore::programsReset);
    QVERIFY(store.load().has_value());
    QVERIFY(store.programs().isEmpty());
    QVERIFY(store.isWritable());
    // Already empty: nothing changed, so nothing is emitted.
    QCOMPARE(aboutToReset.count(), 0);
    QCOMPARE(reset.count(), 0);
}

void TestProgramStore::test_loadRejects_data()
{
    QTest::addColumn<QByteArray>("json");
    QTest::addColumn<QString>("error");
    QTest::addColumn<LoadError::Kind>("kind");

    constexpr auto damaged = LoadError::Kind::Damaged;
    constexpr auto newer = LoadError::Kind::NewerFormat;

    const QByteArray valid = toJson({NamedProgram{.name = "Drums", .program = someProgram()}});
    const auto edited = [&valid](const QByteArray &from, const QByteArray &to) {
        QByteArray json = valid;
        Q_ASSERT(json.contains(from));
        return json.replace(json.indexOf(from), from.size(), to);
    };

    QTest::newRow("Not JSON") << QByteArray("{ nope") << "not valid JSON" << damaged;
    QTest::newRow("Not an object")
            << QByteArray("[]") << "not an lpd8editor programs file" << damaged;
    QTest::newRow("No format") << edited(R"("format": 1)", R"("formats": 1)") << "no format version"
                               << damaged;
    QTest::newRow("Newer format") << edited(R"("format": 1)", R"("format": 2)") << "newer version"
                                  << newer;
    QTest::newRow("Huge format") << edited(R"("format": 1)", R"("format": 1e20)") << "newer version"
                                 << newer;
    QTest::newRow("Format 1.5") << edited(R"("format": 1)", R"("format": 1.5)") << "newer version"
                                << newer;
    QTest::newRow("Format 0.5") << edited(R"("format": 1)", R"("format": 0.5)")
                                << "unknown format version 0.5" << damaged;
    QTest::newRow("Unknown root key") << edited(R"("format": 1)", R"("format": 1, "nextId": 2)")
                                      << "unknown key 'nextId'" << damaged;
    // Ids are not part of the format: one written by hand is a mistake.
    QTest::newRow("An id") << edited(R"("channel": 9)", R"("channel": 9, "id": 1)")
                           << "program 1 (Drums): unknown key 'id'" << damaged;
    QTest::newRow("Unknown program key")
            << edited(R"("channel": 9)", R"("channel": 9, "colour": "red")")
            << "unknown key 'colour'" << damaged;
    QTest::newRow("Typo in a pad key")
            << edited(R"("toggle": false)", R"("toggle": false, "Toggle": true)")
            << "pad 1: unknown key 'Toggle'" << damaged;
    QTest::newRow("Note 300") << edited(R"("note": 36)", R"("note": 300)")
                              << "program 1 (Drums): pad 1: 'note' must be an integer from 0 to 127"
                              << damaged;
    QTest::newRow("Note 36.5") << edited(R"("note": 36)", R"("note": 36.5)")
                               << "'note' must be an integer" << damaged;
    QTest::newRow("Toggle not a boolean") << edited(R"("toggle": false)", R"("toggle": 0)")
                                          << "'toggle' must be true or false" << damaged;
    QTest::newRow("Channel 16") << edited(R"("channel": 9)", R"("channel": 16)")
                                << "'channel' must be an integer from 0 to 15" << damaged;
    QTest::newRow("Name not a string")
            << edited(R"("name": "Drums")", R"("name": 3)") << "'name' must be a string" << damaged;
}

void TestProgramStore::test_loadRejects()
{
    // NOLINTBEGIN(misc-const-correctness): QFETCH declares the variables
    QFETCH(QByteArray, json);
    QFETCH(QString, error);
    QFETCH(LoadError::Kind, kind);
    // NOLINTEND(misc-const-correctness)

    const auto read = fromJson(json);
    QVERIFY(!read.has_value());
    QVERIFY2(read.error().message.contains(error), qPrintable(read.error().message));
    QCOMPARE(read.error().kind, kind);
}

void TestProgramStore::test_failedLoadKeepsStoreAndProtectsFile()
{
    writeAll(path, "{ this is not JSON");

    ProgramStore store(path);
    const int id = store.add(QStringLiteral("Unsaved"), someProgram()).value();
    const QSignalSpy aboutToReset(&store, &ProgramStore::programsAboutToBeReset);
    const QSignalSpy reset(&store, &ProgramStore::programsReset);

    const auto loaded = store.load();
    QVERIFY(!loaded.has_value());
    QCOMPARE(aboutToReset.count(), 0);
    QCOMPARE(reset.count(), 0);
    QVERIFY(hasProgram(store, id)); // unchanged
    QVERIFY(!store.isWritable());
    QCOMPARE(store.loadError().value().message, loaded.error().message);
    QCOMPARE(store.loadError().value().kind, LoadError::Kind::Damaged);
    QCOMPARE(loaded.error().kind, LoadError::Kind::Damaged);

    // Never overwritten.
    QVERIFY(!store.save().has_value());
    QCOMPARE(readAll(path), QByteArray("{ this is not JSON"));
}

void TestProgramStore::test_unreadableFileStaysProtectedUntilReadable()
{
    // The protection belongs to the store's one file: loading again doesn't
    // lift it while the file is still unreadable, and lifts it once the file
    // is repaired.
    writeAll(path, "{ broken");
    ProgramStore store(path);
    QVERIFY(!store.load().has_value());
    QVERIFY(!store.load().has_value());
    QVERIFY(!store.isWritable());
    QVERIFY(!store.save().has_value());
    QCOMPARE(readAll(path), QByteArray("{ broken"));

    writeAll(path, toJson({}));
    QVERIFY(store.load().has_value());
    QVERIFY(store.isWritable());
    QVERIFY(store.save().has_value());
}

void TestProgramStore::test_newerFormatIsNotOverwritten()
{
    const QByteArray newer = R"({"format": 2, "programs": [], "future": "data"})";
    writeAll(path, newer);

    ProgramStore store(path);
    const auto loaded = store.load();
    QVERIFY(!loaded.has_value());
    QCOMPARE(loaded.error().kind, LoadError::Kind::NewerFormat);
    QVERIFY(!store.save().has_value());
    // Not damaged: it stays where the newer version will look for it.
    QVERIFY(!store.setAsideUnreadableFile().has_value());
    QCOMPARE(readAll(path), newer);
}

void TestProgramStore::test_tooLargeFileIsRefused()
{
    writeAll(path, QByteArray(max_store_file_size + 1, ' '));
    ProgramStore store(path);
    const auto loaded = store.load();
    QVERIFY(!loaded.has_value());
    QVERIFY2(loaded.error().message.contains("too large"), qPrintable(loaded.error().message));
    QCOMPARE(loaded.error().kind, LoadError::Kind::Damaged);
    QVERIFY(!store.isWritable());
}

void TestProgramStore::test_saveNeverWritesWhatLoadRefuses()
{
    // load() refuses a file larger than max_store_file_size, and names have
    // no length limit. A store whose file would be larger must not be saved:
    // the next start could not read it, and would then refuse to save over
    // it, locking the user out of all their programs.
    ProgramStore store(path);
    QVERIFY(store.add(QStringLiteral("Drums"), someProgram()).has_value());
    QVERIFY(store.save().has_value());
    const QByteArray saved = readAll(path);

    const QString huge(static_cast<qsizetype>(max_store_file_size), u'x');
    QVERIFY(store.add(huge, someProgram()).has_value());
    const auto written = store.save();
    QVERIFY(!written.has_value());
    QCOMPARE(readAll(path), saved); // the previous file is intact

    // What was saved can always be loaded.
    ProgramStore loaded(path);
    QVERIFY(loaded.load().has_value());
}

void TestProgramStore::test_setAsideKeepsTheBytes()
{
    const QByteArray corrupt = "{ hand-edited, broken";
    writeAll(path, corrupt);

    ProgramStore store(path);
    QVERIFY(!store.load().has_value());
    const QString aside = store.setAsideUnreadableFile().value();
    QVERIFY(aside.startsWith(path + ".corrupt-"));
    QCOMPARE(readAll(aside), corrupt); // the user's bytes, intact
    QVERIFY(!QFile::exists(path));
    QVERIFY(store.isWritable());
    QVERIFY(!store.loadError().has_value());
    QVERIFY(store.save().has_value());

    // Nothing left to set aside.
    QVERIFY(!store.setAsideUnreadableFile().has_value());
}

void TestProgramStore::test_aDirectoryCannotBeRead()
{
    // A directory where the file should be: CannotRead (nothing to set
    // aside), not Damaged. Unlike a permission error, it fails the same way
    // for root, so CI, which may run as root, tests it too.
    QVERIFY(QDir().mkpath(path));
    ProgramStore store(path);
    const auto loaded = store.load();
    QVERIFY(!loaded.has_value());
    QCOMPARE(loaded.error().kind, LoadError::Kind::CannotRead);
    QVERIFY(!store.isWritable());
    QVERIFY(!store.save().has_value());
    QVERIFY(QFileInfo(path).isDir());

    // Not Damaged, so it is not set aside: renaming a directory could hide
    // something the user still needs. Nothing is renamed.
    const QDateTime when(QDate(2026, 1, 2), QTime(3, 4, 5));
    QVERIFY(!store.setAsideUnreadableFile(when).has_value());
    QVERIFY(QFileInfo(path).isDir());
    QVERIFY(!QFileInfo::exists(path + ".corrupt-20260102-030405"));
    QVERIFY(!store.isWritable());
}

void TestProgramStore::test_aFifoDoesNotBlock()
{
    // Opening a FIFO for reading blocks until a writer comes: the store
    // must not even try. CannotRead, at once.
    QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
    QCOMPARE(::mkfifo(QFile::encodeName(path).constData(), 0600), 0);
    ProgramStore store(path);
    const auto loaded = store.load();
    QVERIFY(!loaded.has_value());
    QCOMPARE(loaded.error().kind, LoadError::Kind::CannotRead);
    QVERIFY(!store.isWritable());
    QVERIFY(!store.save().has_value());
    QVERIFY(!store.setAsideUnreadableFile().has_value());
    // Still the FIFO, not renamed.
    QVERIFY(QFileInfo::exists(path));
    QVERIFY(!QFileInfo(path).isFile());
}

void TestProgramStore::test_aFolderThatCannotBeEnteredIsNotAFirstStart()
{
    // stat() fails with EACCES, not ENOENT. Taking that for a missing
    // file would start empty and, at the first save attempt, report a
    // confusing error, or worse, write elsewhere. Root ignores permissions,
    // so there the file is simply read and this test can't provoke the
    // error.
    if (::geteuid() == 0) {
        QSKIP("root can enter any folder");
    }
    const QString locked = dir->filePath("locked");
    QVERIFY(QDir().mkpath(locked));
    const QString inside = locked + "/programs.json";
    writeAll(inside, toJson({}));
    QVERIFY(QFile::setPermissions(locked, QFileDevice::Permissions{}));
    // Restored even if a check fails, so QTemporaryDir can remove it.
    const auto restore = qScopeGuard([&locked] {
        QFile::setPermissions(
                locked, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    });

    ProgramStore store(inside);
    const auto loaded = store.load();
    QVERIFY(!loaded.has_value());
    QCOMPARE(loaded.error().kind, LoadError::Kind::CannotRead);
    QVERIFY(!store.isWritable());
    QVERIFY(!store.setAsideUnreadableFile().has_value());
}

void TestProgramStore::test_setAsideRefusesATakenName()
{
    // QFile::rename() never replaces an existing file. With the
    // timestamp fixed, the target name can be taken in advance: the rename
    // fails, for root too (unlike a permission error), and nothing is
    // lost.
    const QByteArray corrupt = "{ broken";
    writeAll(path, corrupt);
    const QDateTime when(QDate(2026, 1, 2), QTime(3, 4, 5));
    const QString taken = path + ".corrupt-20260102-030405";
    writeAll(taken, "an earlier one");

    ProgramStore store(path);
    QVERIFY(!store.load().has_value());
    const auto aside = store.setAsideUnreadableFile(when);
    QVERIFY(!aside.has_value());
    QVERIFY(aside.error().contains(QDir::toNativeSeparators(taken)));
    QCOMPARE(readAll(path), corrupt);
    QCOMPARE(readAll(taken), QByteArray("an earlier one"));
    QVERIFY(!store.isWritable());

    // A second later, the name is free.
    const QString moved = store.setAsideUnreadableFile(when.addSecs(1)).value();
    QCOMPARE(moved, path + ".corrupt-20260102-030406");
    QCOMPARE(readAll(moved), corrupt);
}

void TestProgramStore::test_saveCreatesTheFolder()
{
    ProgramStore store(dir->filePath("not/yet/there/programs.json"));
    store.add(QStringLiteral("A"), someProgram());
    QVERIFY(store.save().has_value());
    QVERIFY(QFile::exists(store.path()));
}

void TestProgramStore::test_saveReportsFailure()
{
    // The parent of the path is a regular file, so the save must fail, for
    // root too (CI containers run as root, and root ignores permissions).
    const QString notAFolder = dir->filePath("a-regular-file");
    writeAll(notAFolder, "x");

    ProgramStore store(notAFolder + "/programs.json");
    store.add(QStringLiteral("A"), someProgram());
    const auto saved = store.save();
    QVERIFY(!saved.has_value());
    QVERIFY2(saved.error().startsWith("Cannot"), qPrintable(saved.error()));
}

QTEST_GUILESS_MAIN(TestProgramStore)

#include "programstore_test.moc"
