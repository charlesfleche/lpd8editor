#include "utils.h"

#include <QDir>
#include <QStandardPaths>
#include <QTextStream>

#include <QtDebug>

QDir dataDir() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
}

bool initFilesystem() {
    return dataDir().mkpath(".");
}

QString defaultDbPath() {
    return dataDir().filePath("db.sqlite3");
}

bool checkValue(int v, int min, int max) {
    return v >= min && v <= max;
}

#define READ_CHECK_OR_RETURN(in, v, min, max, path) \
    in >> v; \
    if (in.atEnd() || !checkValue(v, min, max)) { \
        qWarning() << "Failed to parse" << path; \
        return pProgram(); \
    }

pProgram readProgramFile(const QString& path) {
    pProgram ret(new Program);

    QFile data(path);
    if (!data.open(QFile::ReadOnly | QFile::Truncate)) {
        qWarning() << "Failed to open" << path;
        return pProgram();
    }

    QTextStream in(&data);
    int v;

    // Header

    in >> v; // 240
    in >> v; // 71
    in >> v; // 127
    in >> v; // 117
    in >> v; // 97
    in >> v; // 0
    in >> v; // 58

    READ_CHECK_OR_RETURN(in, v, 1, 4, path);
    ret->id = static_cast<char>(v);

    READ_CHECK_OR_RETURN(in, v, 0, 15, path);
    ++v;
    ret->channel = static_cast<char>(v);

    for (int i = 0 ; i < 8 ; ++i) {
        READ_CHECK_OR_RETURN(in, v, 0, 127, path);
        ret->pads[i].note = static_cast<char>(v);

        READ_CHECK_OR_RETURN(in, v, 0, 127, path);
        ret->pads[i].pc = static_cast<char>(v);

        READ_CHECK_OR_RETURN(in, v, 0, 127, path);
        ret->pads[i].cc = static_cast<char>(v);

        READ_CHECK_OR_RETURN(in, v, 0, 1, path);
        ret->pads[i].toggle = static_cast<char>(v);
    }

    for (int i = 0 ; i < 8 ; ++i) {
        READ_CHECK_OR_RETURN(in, v, 0, 127, path);
        ret->knobs[i].cc = static_cast<char>(v);

        READ_CHECK_OR_RETURN(in, v, 0, 127, path);
        ret->knobs[i].low = static_cast<char>(v);

        READ_CHECK_OR_RETURN(in, v, 0, 127, path);
        ret->knobs[i].high = static_cast<char>(v);
    }

    return ret;
}
