#ifndef SQLITE_SCHEMA_H
#define SQLITE_SCHEMA_H

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

#endif // SQLITE_SCHEMA_H
