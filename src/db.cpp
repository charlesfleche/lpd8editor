#include "db.h"

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
    if(!q.prepare("delete from programs where programId = ?")) return false;
    q.addBindValue(programId);
    return q.exec();
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
//    if (!q.exec(readTextFile(":/create_trigger_add_program.sql"))) return false;
//    if (!q.exec(readTextFile(":/create_trigger_delete_program.sql"))) return false;
    return true;
}

QSqlError initDb(const QString& path) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
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
