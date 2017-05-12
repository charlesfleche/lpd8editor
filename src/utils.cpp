#include "utils.h"

#include <QAbstractItemModel>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>

#include <QtDebug>

QDir dataDir() {
    QStandardPaths::StandardLocation location = QStandardPaths::DataLocation;
#if QT_VERSION >= 0x050400
    location = QStandardPaths::AppDataLocation;
#endif
    return QDir(QStandardPaths::writableLocation(location));
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

#define WRITE_CHECK_OR_RETURN(out, v, path) \
    out << static_cast<int>(v) << " "; \
    if (out.status() != QTextStream::Ok) { \
        qWarning() << "Failed to write" << path; \
        return; \
    }

void writeProgramFile(pProgram p, const QString &path) {
    Q_CHECK_PTR(p);

    QFile data(path);
    if (!data.open(QFile::WriteOnly | QFile::Truncate)) {
        qWarning() << "Failed to open" << path;
        return;
    }

    QTextStream out(&data);

    // Header

    WRITE_CHECK_OR_RETURN(out, 240, path);
    WRITE_CHECK_OR_RETURN(out, 71, path);
    WRITE_CHECK_OR_RETURN(out, 127, path);
    WRITE_CHECK_OR_RETURN(out, 117, path);
    WRITE_CHECK_OR_RETURN(out, 97, path);
    WRITE_CHECK_OR_RETURN(out, 0, path);
    WRITE_CHECK_OR_RETURN(out, 58, path);

    WRITE_CHECK_OR_RETURN(out, p->id, path);
    WRITE_CHECK_OR_RETURN(out, p->channel, path);

    for (int i = 0 ; i < 8 ; ++i) {
        WRITE_CHECK_OR_RETURN(out, p->pads[i].note, path);
        WRITE_CHECK_OR_RETURN(out, p->pads[i].pc, path);
        WRITE_CHECK_OR_RETURN(out, p->pads[i].cc, path);
        WRITE_CHECK_OR_RETURN(out, p->pads[i].toggle, path);
    }
    for (int i = 0 ; i < 8 ; ++i) {
        WRITE_CHECK_OR_RETURN(out, p->knobs[i].cc, path);
        WRITE_CHECK_OR_RETURN(out, p->knobs[i].low, path);
        WRITE_CHECK_OR_RETURN(out, p->knobs[i].high, path);
    }

    // Footer

    WRITE_CHECK_OR_RETURN(out, 247, path);
}

int getProgramId(const QAbstractItemModel* model, int row)
{
    Q_CHECK_PTR(model);
    Q_ASSERT(row >= 0);

    return model->data(model->index(row, 0)).toInt();
}
