#include "utils.h"

#include <QDir>
#include <QStandardPaths>

QDir dataDir() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
}

bool initFilesystem() {
    return dataDir().mkpath(".");
}

QString defaultDbPath() {
    return dataDir().filePath("db.sqlite3");
}
