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

bool addProgram(const QString& name) {
    QSqlQuery q;
    if (!q.prepare("insert into programs(name, channel) values(?, ?)")) return false;
    q.addBindValue(name);
    q.addBindValue(10);
    return q.exec();
}

bool addProgram(int programId) {
    QSqlQuery q;
    if (!q.prepare("insert into programs(programId, name, channel) values(?, ?, ?)")) return false;
    q.addBindValue(programId);
    q.addBindValue("default");
    q.addBindValue(10);
    return q.exec();
}

bool deleteProgram(int programId) {
    Q_ASSERT(programId > 0);

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

    qDebug() << q.size();
    while (q.next()) {
        qDebug() << q.value(0);
        if (programId == q.value(0)) {
            return true;
        }
    }
    return false;
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
    if (isValidProgramId(0)) {
        return true;
    }
    return addProgram(0);
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
