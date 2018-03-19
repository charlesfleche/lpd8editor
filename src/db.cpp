#include "db.h"

#include "lpd8_sysex.h"
#include "utils.h"

#include <exception>

QString sqlErrorMessage(const QString& msg, const QSqlError& err) {
    return msg + QString("\n") + err.text();
}

bool isInitialized(const QSqlDatabase& db) {
    return db.tables().contains("programs");
}

QString programName(const QString& name = QString()) {
    return name.isEmpty() ? "default" : name;
}

#define GOTO_END_IF_FALSE(X) if (!X) goto end;

QList<int> programIds() {
    QList<int> ret;

    QSqlQuery q;
    GOTO_END_IF_FALSE(q.prepare("select programId from programs order by programId"));
    GOTO_END_IF_FALSE(q.exec());

    while(q.next()) {
        ret += q.value(0).toInt();
    }

end:
    if (q.lastError().isValid()) {
        qWarning() << "Failed to retrieve a list of program ids:" << q.lastError().text();
    }
    return ret;
}

int programChannel(int programId) {
    int ret = -1;
    QSqlQuery q;
    GOTO_END_IF_FALSE(q.prepare("select channel from programs where programId = ?"));
    q.addBindValue(programId);
    GOTO_END_IF_FALSE(q.exec());

    GOTO_END_IF_FALSE(q.first());
    ret = q.value(0).toInt();

    Q_ASSERT(!q.next());

end:
    if (q.lastError().isValid()) {
        qWarning() << "Failed to retrieve channel for program" << programId << ":" << q.lastError().text();
    }
    return ret;
}

bool setProgramChannel(int programId, int channel) {
    bool ret = false;
    QSqlQuery q;
    GOTO_END_IF_FALSE(q.prepare("update programs set channel = ? where programId = ?"));
    q.addBindValue(channel);
    q.addBindValue(programId);
    ret = q.exec();

end:
    if (q.lastError().isValid()) {
        qWarning() << "Failed to set channel" << channel << "for program" << programId << ":" << q.lastError().text();
    }
    return ret;
}

QString programName(int programId) {
    QString ret;
    QSqlQuery q;
    GOTO_END_IF_FALSE(q.prepare("select name from programs where programId = ?"));
    q.addBindValue(programId);
    GOTO_END_IF_FALSE(q.exec());

    GOTO_END_IF_FALSE(q.first());
    ret = q.value(0).toString();

    Q_ASSERT(!q.next());

end:
    if (q.lastError().isValid()) {
        qWarning() << "Failed to retrieve program" << programId << "name:" << q.lastError().text();
    }
    return ret;
}

QByteArray programSysex(int programId) {
    QByteArray ret;
    sysex::addProgramHeader(ret, 1);

    int count = 0;

    QSqlQuery q;

    // channel

    GOTO_END_IF_FALSE(q.prepare("select channel from programs where programId = ?"));
    q.addBindValue(programId);
    GOTO_END_IF_FALSE(q.exec());

    GOTO_END_IF_FALSE(q.first());
    ret += q.value(0).toChar();

    Q_ASSERT(!q.next());

    // pads

    GOTO_END_IF_FALSE(q.prepare("select note, pc, cc, toggle from pads where programId = ? order by controlId"));
    q.addBindValue(programId);
    GOTO_END_IF_FALSE(q.exec());

    count = 0;
    while(q.next()) {
        const QSqlRecord r = q.record();
        for (int i = 0 ; i < r.count() ; ++i) {
            ret += r.value(i).toChar();
        }
        ++count;
    }

    Q_ASSERT(count == sysex::padsCount());

    // knobs

    GOTO_END_IF_FALSE(q.prepare("select cc, low, high from knobs where programId = ? order by controlId"));
    q.addBindValue(programId);
    GOTO_END_IF_FALSE(q.exec());

    count = 0;
    while(q.next()) {
        const QSqlRecord r = q.record();
        for (int i = 0 ; i < r.count() ; ++i) {
            ret += r.value(i).toChar();
        }
        ++count;
    }

    Q_ASSERT(count == sysex::knobsCount());

    sysex::addFooter(ret);

end:
    if (q.lastError().isValid()) {
        qWarning() << "Failed to generate sysex for program" << programId << ":" << q.lastError().text();
        ret.clear();
    }
    return ret;
}

int createProgram(const QString &name, const QByteArray &sysex) {
    Q_UNUSED(name);
    Q_UNUSED(sysex);

    int programId = -1;

    QSqlDatabase::database().transaction();

    QSqlQuery q;
    GOTO_END_IF_FALSE(q.prepare("insert into programs default values;"));
    GOTO_END_IF_FALSE(q.exec());

    programId = q.lastInsertId().toInt();

    GOTO_END_IF_FALSE(q.prepare("update programs set name = ? where programId = ?"));
    q.addBindValue(name.isNull() ? QVariant(programId) : QVariant(name));
    q.addBindValue(programId);
    GOTO_END_IF_FALSE(q.exec());

    // XXX this is a lame check
    // Proper way would be to pass a QIODevice and register callbacks
    // ala SAX
    if (!sysex.isNull()) {
        QDataStream s(sysex);
        qint8 v = 0;

        // Header

        s >> v; // Sysex Start
        s >> v; // Manufacturer
        s >> v; // Model byte 1
        s >> v; // Model byte 2
        s >> v; // Opcode set program: byte1
        s >> v; // Opcode set program: byte2
        s >> v; // Opcode set program: byte3
        s >> v; // Program ID

        // Program

        s >> v;
        GOTO_END_IF_FALSE(q.prepare("update programs set channel = ? where programId = ?"));
        q.addBindValue(v);
        q.addBindValue(programId);
        GOTO_END_IF_FALSE(q.exec());

        // Pads

        for (int controlId = 0; controlId < sysex::padsCount(); ++controlId) {
            GOTO_END_IF_FALSE(q.prepare("update pads set note = ?, pc = ?, cc = ?, toggle = ? where programId = ? and controlId = ?"));
            s >> v;
            q.addBindValue(v);
            s >> v;
            q.addBindValue(v);
            s >> v;
            q.addBindValue(v);
            s >> v;
            q.addBindValue(v);
            q.addBindValue(programId);
            q.addBindValue(controlId);
            GOTO_END_IF_FALSE(q.exec());
        }

        for (int controlId = 0; controlId < sysex::knobsCount(); ++controlId) {
            GOTO_END_IF_FALSE(q.prepare("update knobs set cc = ?, low = ?, high = ? where programId = ? and controlId = ?"));
            s >> v;
            q.addBindValue(v);
            s >> v;
            q.addBindValue(v);
            s >> v;
            q.addBindValue(v);
            q.addBindValue(programId);
            q.addBindValue(controlId);
            GOTO_END_IF_FALSE(q.exec());
        }
    }

end:
    if (q.lastError().isValid()) {
        qWarning() << "Failed to create program: " << q.lastError().text();
    }
    if (!QSqlDatabase::database().commit()) {
        qWarning() << "Failed to commit program creation transaction:" << QSqlDatabase::database().lastError().text();
    }
    return programId;
}

bool addProgram(const QString& name, int& programId) {
    QSqlQuery q;
    if (!q.prepare("insert into programs(name, channel) values(?, ?)")) return false;
    q.addBindValue(programName(name));
    q.addBindValue(10);
    if (!q.exec()) {
        return false;
    }
    programId = q.lastInsertId().toInt();
    return true;
}

bool addProgram(int programId) {
    QSqlQuery q;
    if (!q.prepare("insert into programs(programId, name, channel) values(?, ?, ?)")) return false;
    q.addBindValue(programId);
    q.addBindValue(programName());
    q.addBindValue(10);
    return q.exec();
}

bool deleteProgram(int programId) {
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
    if (q.lastError().isValid()) {
        qWarning() << "Failed to delete program: " << q.lastError().text();
        return false;
    }
    return true;
}

bool programsIds(QList<int>& ids) {
    QSqlQuery q;
    if (!q.exec("select programId from programs;")) {
        qWarning() << "Cannot get programs IDs";
        qWarning() << q.lastError().text();
        return false;
    }

    ids.clear();
    // sqlite driver doesn't support query sizes
    while (q.next()) {
        ids.append(q.value(0).toInt());
    }
    return true;
}

int nextProgramId(int programId) {
    QList<int> ids;
    if (!programsIds(ids)) {
        return -1;
    }
    int i = ids.indexOf(programId);
    if (i == -1 || ids.count() == 1) {
        return -1;
    }
    if (i > 0) {
        return ids[i-1];
    }
    return ids[i+1];
}

bool setProgramName(int programId, const QString& name) {
    QSqlQuery q;
    if(!q.prepare("update programs set name = ? where programId = ?")) return false;
    q.addBindValue(name);
    q.addBindValue(programId);
    return q.exec();
}

bool isValidProgramId(int programId) {
    QList<int> ids;
    return programsIds(ids) ? ids.contains(programId) : false;
}

bool initialize() {
    QSqlQuery q;
    if (!q.exec(readTextFile(":/create_table_programs.sql"))) return false;
    if (!q.exec(readTextFile(":/create_table_pads.sql"))) return false;
    if (!q.exec(readTextFile(":/create_table_knobs.sql"))) return false;
    if (!q.exec(readTextFile(":/create_trigger_add_program.sql"))) return false;
    if (!q.exec(readTextFile(":/create_trigger_delete_program.sql"))) return false;
    return true;
}

QSqlError initDb(const QString& path) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    QSqlDriver *driver = db.driver();

    Q_CHECK_PTR(driver);
    Q_ASSERT(driver->hasFeature(QSqlDriver::Transactions));
    Q_ASSERT(driver->hasFeature(QSqlDriver::Unicode));
    Q_ASSERT(driver->hasFeature(QSqlDriver::PreparedQueries));
    Q_ASSERT(driver->hasFeature(QSqlDriver::PositionalPlaceholders));
    Q_ASSERT(driver->hasFeature(QSqlDriver::LastInsertId));
    Q_ASSERT(driver->hasFeature(QSqlDriver::EventNotifications));

    db.setDatabaseName(path);

    if (!db.open()) {
        qWarning() << "Failed to open db" << path;
        goto end;
    }

    if (!isInitialized(db)) {
        if (!initialize()) {
            qWarning() << "Failed to initialize db" << path;
            goto end;
        }
    }

    end:
    return db.lastError();
}
