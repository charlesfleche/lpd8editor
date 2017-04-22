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
    return db.tables().contains("presets");
}

bool addPreset(const QString& name) {
    QSqlQuery q;
    if (!q.prepare("insert into presets(name) values(?)")) return false;
    q.addBindValue(name);
    return q.exec();
}

bool addPreset(int presetId) {
    QSqlQuery q;
    if (!q.prepare("insert into presets(presetId, name) values(?, ?)")) return false;
    q.addBindValue(presetId);
    q.addBindValue("default");
    return q.exec();
}

bool deletePreset(int presetId) {
    Q_ASSERT(presetId > 0);

    QSqlQuery q;
    if(!q.prepare("delete from presets where presetId = ?")) return false;
    q.addBindValue(presetId);
    return q.exec();
}

bool setPresetName(int presetId, const QString& name) {
    QSqlQuery q;
    if(!q.prepare("update presets set name = ? where presetId = ?")) return false;
    q.addBindValue(name);
    q.addBindValue(presetId);
    return q.exec();
}

bool isValidPresetId(int presetId) {
    QSqlQuery q;
    if (!q.exec("select presetId from presets;")) {
        qWarning() << "Cannot select presets";
        qWarning() << q.lastError().text();
        return false;
    }
    for (int i = 0 ; i < q.size() ; ++i) {
        if (presetId == q.value(i)) {
            return true;
        }
    }
    return false;
}

bool isValidProgramId(int programId) {
    return programId >= 1 && programId <= 4;
}

bool initialize() {
    QSqlQuery q;
    if (!q.exec(readTextFile(":/create_table_presets.sql"))) return false;
    if (!q.exec(readTextFile(":/create_table_pads.sql"))) return false;
    if (!q.exec(readTextFile(":/create_table_knobs.sql"))) return false;
    if (!q.exec(readTextFile(":/create_trigger_add_preset.sql"))) return false;
    if (!q.exec(readTextFile(":/create_trigger_delete_preset.sql"))) return false;
    return true;
}

bool ensureDefaultPreset() {
    if (!isValidPresetId(0)) {
        return addPreset(0);
    }
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

    if (!ensureDefaultPreset()) {
        qWarning() << "Failed to ensure default preset in db" << path;
        goto end;
    }

    end:
    return db.lastError();
}
