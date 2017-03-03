#include "db.h"

bool isInitialized(const QSqlDatabase& db) {
    return db.tables().contains("presets");
}

bool addPad(int presetId, int programId, int controlId) {
    QSqlQuery q;
    if (!q.prepare("insert into pads( presetId,  programId,  controlId,  note,  pc,  cc,  momentary)"
                             "values(:presetId, :programId, :controlId, :note, :pc, :cc, :momentary)")) return false;
    q.bindValue(":presetId", presetId);
    q.bindValue(":programId", programId);
    q.bindValue(":controlId", controlId);
    q.bindValue(":note", 1);
    q.bindValue(":pc", 2);
    q.bindValue(":cc", 3);
    q.bindValue(":momentary", 0);
    return q.exec();
}

bool addKnob(int presetId, int programId, int controlId) {
    QSqlQuery q;
    if (!q.prepare("insert into knobs( presetId,  programId,  controlId,  cc,  value)"
                              "values(:presetId, :programId, :controlId, :cc, :value)")) return false;
    q.bindValue(":presetId", presetId);
    q.bindValue(":programId", programId);
    q.bindValue(":controlId", controlId);
    q.bindValue(":cc", 4);
    q.bindValue(":value", 5);
    return q.exec();
}

bool addPreset(const QString& name) {
    QSqlQuery q;
    if (!q.prepare("insert into presets(name) values(?)")) return false;
    q.addBindValue(name);
    if (!q.exec()) return false;

    int presetId = q.lastInsertId().toInt();

    for (int programId = 0 ; programId < 4 ; ++programId) {
        for (int controlId = 1 ; controlId <= 8 ; ++controlId) {
            if (!addPad(presetId, programId, controlId)) return false;
            if (!addKnob(presetId, programId, controlId)) return false;
        }
    }
    return true;
}

bool initialize() {
    QSqlQuery q;
    if (!q.exec("create table presets(presetId integer primary key, name varchar)")) return false;
    if (!q.exec("create table pads(presetId integer,"
                                  "programId integer NOT NULL,"
                                  "controlId integer NOT NULL,"
                                  "note integer NOT NULL,"
                                  "pc integer NOT NULL,"
                                  "cc integer NOT NULL,"
                                  "momentary integer NOT NULL,"
                                  "PRIMARY KEY (presetId, programId, controlId),"
                                  "FOREIGN KEY (presetId) REFERENCES presets (presetId)"
                                  "ON DELETE CASCADE ON UPDATE NO ACTION"
                                  ");")) return false;
    if (!q.exec("create table knobs(presetId integer,"
                                   "programId integer NOT NULL,"
                                   "controlId integer NOT NULL,"
                                   "cc integer NOT NULL,"
                                   "value integer NOT NULL,"
                                   "PRIMARY KEY (presetId, programId, controlId),"
                                   "FOREIGN KEY (presetId) REFERENCES presets (presetId)"
                                   "ON DELETE CASCADE ON UPDATE NO ACTION"
                                   ");")) return false;

    if(!addPreset("preset0")) return false;
    if(!addPreset("preset1")) return false;
    if(!addPreset("preset2")) return false;

    return true;
}

QSqlError initDb(const QString& path) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    if (db.open() && !isInitialized(db)) {
        if (!initialize()) {
            qDebug() << "Failed to initialize db";
        }
    }
    return db.lastError();
}
