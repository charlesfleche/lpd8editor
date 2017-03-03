#include "db.h"

bool isInitialized(const QSqlDatabase& db) {
    return db.tables().contains("presets");
}

QVariant addPreset(QSqlQuery& q, const QString& name) {
    q.addBindValue(name);
    if (!q.exec()) return QVariant();
    return q.lastInsertId();
}

void initialize() {
    QSqlQuery q;
    if (!q.exec("create table presets(id integer primary key, name varchar)")) return;
    if (!q.prepare("insert into presets(name) values(?)")) return;
    if( !addPreset(q, "preset0").isValid()) return;
    if( !addPreset(q, "preset1").isValid()) return;
    if( !addPreset(q, "preset2").isValid()) return;
}

QSqlError initDb(const QString& path) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    if (db.open() && !isInitialized(db)) {
        initialize();
    }
    return db.lastError();
}
