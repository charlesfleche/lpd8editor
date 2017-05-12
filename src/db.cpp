#include "db.h"

#include <exception>

QString readTextFile(const QString& path) {
    QFile f(path);
    if (!f.open(QFile::ReadOnly | QFile::Text)) {
        QString msg("Failed to read file: ");
        msg += path;
        throw std::runtime_error(msg.toStdString());
    }
    QTextStream ts(&f);
    return ts.readAll();
}

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

bool setProgramName(int programId, const QString& name) {
    QSqlQuery q;
    if(!q.prepare("update programs set name = ? where programId = ?")) return false;
    q.addBindValue(name);
    q.addBindValue(programId);
    return q.exec();
}

bool isValidProgramId(int programId) {
    QSqlQuery q;
    if (!q.exec("select programId from programs;")) {
        qWarning() << "Cannot select programs";
        qWarning() << q.lastError().text();
        return false;
    }

    while (q.next()) {
        if (programId == q.value(0)) {
            return true;
        }
    }
    return false;
}

int programsCount() {
    QSqlQuery q;
    if (!q.exec("select programId from programs;")) {
        qWarning() << "Cannot select programs";
        qWarning() << q.lastError().text();
        return false;
    }
    int ret = 0;

    // sqlite driver doesn't support query sizes
    while (q.next()) {
        ++ret;
    }
    return ret;
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

bool ensureDefaultProgram() {
    if (programsCount() > 0) {
        return true;
    }
    int programId;
    return addProgram(programName(), programId);
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

    if (!ensureDefaultProgram()) {
        qWarning() << "Failed to ensure default program in db" << path;
        goto end;
    }

    end:
    return db.lastError();
}
