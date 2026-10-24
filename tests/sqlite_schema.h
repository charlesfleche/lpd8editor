#ifndef SQLITE_SCHEMA_H
#define SQLITE_SCHEMA_H

#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QtDebug>

// The SQLite schema the application created until item 4.7b, word for word
// from the .sql files deleted then (git show 3824a69:src/). The migration
// reads databases made with it, so its tests build them the same way. It
// has the CHECK constraints added in v0.0.6, and triggers that insert the
// default pads and knobs of a new program.
inline const QStringList &lastSqliteSchema()
{
    static const QStringList statements{
            R"(create table programs(programId integer primary key autoincrement,
                      name varchar not null default 'default',
                      channel integer not null default 9 check (channel >= 0 AND channel <= 15));)",
            R"(create table pads(programId integer NOT NULL,
                  controlId integer NOT NULL check (controlId >= 1 and controlId <= 8),
                  note integer NOT NULL check (note >= 0 and note <= 127),
                  pc integer NOT NULL check (pc >= 0 and pc <= 127),
                  cc integer NOT NULL check (cc >= 0 and cc <= 127),
                  toggle integer NOT NULL check (toggle >= 0 and toggle <= 1),
                  PRIMARY KEY (programId, controlId),
                  FOREIGN KEY (programId) REFERENCES programs (programId));)",
            R"(create table knobs(programId integer NOT NULL,
                   controlId integer NOT NULL check (controlId >= 1 and controlId <= 8),
                   cc integer NOT NULL check (cc >= 0 and cc <= 127),
                   low integer NOT NULL check (low >= 0 and low <= 127),
                   high integer NOT NULL check (high >= 0 and high <= 127),
                   PRIMARY KEY (programId, controlId),
                   FOREIGN KEY (programId) REFERENCES programs (programId));)",
            R"(create trigger aft_insert after insert on programs
begin

insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 1, 36, 1, 48, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 2, 37, 2, 49, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 3, 38, 3, 50, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 4, 39, 4, 51, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 5, 40, 5, 16, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 6, 41, 6, 17, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 7, 42, 7, 18, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 8, 43, 8, 19, 0);

insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 1, 16, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 2, 17, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 3, 18, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 4, 19, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 5, 48, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 6, 49, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 7, 50, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 8, 51, 0, 127);

end;)",
            R"(create trigger aft_delete after delete on programs
begin
delete from pads where programId = OLD.programId;
delete from knobs where programId = OLD.programId;
end;)",
    };
    return statements;
}

// Creates that schema in 'db', which is open and empty. False, with a
// warning, if a statement fails.
inline bool createLastSqliteSchema(const QSqlDatabase &db)
{
    QSqlQuery q(db);
    for (const QString &statement : lastSqliteSchema()) {
        if (!q.exec(statement)) {
            qWarning() << "Cannot create the schema:" << q.lastError().text();
            return false;
        }
    }
    return true;
}

// A write to the database at 'path' (closed, any schema), left in
// progress: a transaction whose changes were spilled into the database
// file, with the old pages saved in '<path>-journal', and SQLite's lock
// held, until the object is destroyed (then it rolls back). ok() is false,
// with a warning, if a step failed.
//
// How: with a one-page cache, a large update can't keep its changes in
// memory, so SQLite writes them to the file before the commit, after saving
// the old pages in the journal.
class InterruptedWrite
{
public:
    explicit InterruptedWrite(const QString &path)
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connection);
        db.setDatabaseName(path);
        m_ok = db.open();
        exec("create table if not exists filler(x blob)");
        for (int i = 0; i < 50; ++i) {
            exec("insert into filler values (randomblob(4000))");
        }
        exec("pragma cache_size = 1");
        exec("begin");
        exec("update filler set x = randomblob(4000)");
        if (m_ok && QFileInfo(path + "-journal").size() == 0) {
            qWarning() << "No journal was written";
            m_ok = false;
        }
    }
    ~InterruptedWrite()
    {
        {
            QSqlDatabase db = QSqlDatabase::database(connection);
            QSqlQuery(db).exec("rollback");
            db.close();
        }
        QSqlDatabase::removeDatabase(connection);
    }
    InterruptedWrite(const InterruptedWrite &) = delete;
    InterruptedWrite &operator=(const InterruptedWrite &) = delete;
    InterruptedWrite(InterruptedWrite &&) = delete;
    InterruptedWrite &operator=(InterruptedWrite &&) = delete;

    bool ok() const { return m_ok; }

private:
    static constexpr const char *connection = "interrupted-write";

    void exec(const QString &statement)
    {
        QSqlQuery q(QSqlDatabase::database(connection));
        if (m_ok && !q.exec(statement)) {
            qWarning() << statement << q.lastError().text();
            m_ok = false;
        }
    }

    bool m_ok{false};
};

// Makes 'to' a copy of the database at 'from' as a crash would leave it: in
// the middle of a write, with a 'hot journal' ('<to>-journal') that SQLite
// must play back before anyone can read it. 'from' is left as it was. The
// copy's journal is "hot" because no connection holds a lock on the copy:
// the writer is gone, as after a crash.
inline bool copyWithHotJournal(const QString &from, const QString &to)
{
    const InterruptedWrite write(from);
    return write.ok() && QFile::copy(from, to) && QFile::copy(from + "-journal", to + "-journal");
}

#endif // SQLITE_SCHEMA_H
