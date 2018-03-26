#include "utils.h"

#include <QAbstractItemModel>
#include <QBuffer>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>

#include <QtDebug>

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

char readAndValidateChar(QTextStream& in, char expected) {
    if (in.atEnd()) {
        throw std::runtime_error("Trying to read from an ended text stream");
    }
    int c = 0;
    in >> c;
    const char ret = static_cast<char>(c);
    if (ret != expected) {
        throw std::runtime_error("Check failed");
    }
    return static_cast<char>(ret);
}

char readAndValidateChar(QTextStream& in, char min, char max) {
    if (in.atEnd()) {
        throw std::runtime_error("Trying to read from an ended text stream");
    }
    int c = 0;
    in >> c;
    const char ret = static_cast<char>(c);
    if (ret < min && ret > max) {
        throw std::runtime_error("Check failed");
    }
    return static_cast<char>(ret);
}

char readAndValidateChar(QTextStream& in, const QList<char>& allowed) {
    if (in.atEnd()) {
        throw std::runtime_error("Trying to read from an ended text stream");
    }
    int c = 0;
    in >> c;
    const char ret = static_cast<char>(c);
    if (!allowed.contains(ret)) {
        throw std::runtime_error("Check failed");
    }
    return static_cast<char>(ret);
}

QByteArray fromSysexTextFile(const QString &path) {
    QByteArray ret;

    QFile data(path);
    try {
        if (!data.open(QFile::ReadOnly | QFile::Truncate)) {
            throw std::runtime_error("Cannot open file");
        }

        QTextStream in(&data);

        // Header

        ret.append(readAndValidateChar(in, 0xf0)); // Sysex Start
        ret.append(readAndValidateChar(in, 0x47)); // Manufacturer
        ret.append(readAndValidateChar(in, 0x7f)); // Model byte 1
        ret.append(readAndValidateChar(in, 0x75)); // Model byte 2
        ret.append(readAndValidateChar(in, 0x61)); // Opcode set program: byte1
        ret.append(readAndValidateChar(in, 0x00)); // Opcode set program: byte2
        ret.append(readAndValidateChar(in, 0x3a)); // Opcode set program: byte3
        ret.append(readAndValidateChar(in, 1, 4)); // program id
        ret.append(readAndValidateChar(in, 0, 15)); // midi channel

        for (int i = 0 ; i < 8 ; ++i) {
            ret.append(readAndValidateChar(in, 0, 127)); // pad node
            ret.append(readAndValidateChar(in, 0, 127)); // pad pc
            ret.append(readAndValidateChar(in, 0, 127)); // pad cc
            ret.append(readAndValidateChar(in, 0, 1));   // pad toggle
        }

        for (int i = 0 ; i < 8 ; ++i) {
            ret.append(readAndValidateChar(in, 0, 127)); // knob cc
            ret.append(readAndValidateChar(in, 0, 127)); // knob low
            ret.append(readAndValidateChar(in, 0, 127)); // knob high
        }

        ret.append(readAndValidateChar(in, 0xf7)); // Sysex End
    } catch (const std::runtime_error& e) {
        qWarning() << "Failed to parse sysex text file:" << path << "[" << e.what() << "]";
        return QByteArray();
    }

    return ret;
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

    in >> v; // 0xF0 240 Sysex Start
    in >> v; // 0x47 71  Manufacturer
    in >> v; // 0x7F 127 Model byte 1
    in >> v; // 0x75 117 Model byte 2
    in >> v; // 0x61 97  Opcode set program: byte1
    in >> v; // 0x00 0   Opcode set program: byte2
    in >> v; // 0x3A 58  Opcode set program: byte3

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

void writeProgramFile(const QByteArray &sysex, const QString &path) {
    QFile f(path);
    if (!f.open(QFile::WriteOnly | QFile::Truncate)) {
        qWarning() << "Failed to open" << path;
        return;
    }

    QTextStream out(&f);

    for (auto it = sysex.constBegin() ; it != sysex.constEnd() ; ++it) {
        out << static_cast<unsigned char>(*it) << " ";
    }
}

int getProgramId(const QAbstractItemModel* model, int row)
{
    Q_CHECK_PTR(model);
    Q_ASSERT(row >= 0);

    return model->data(model->index(row, 0)).toInt();
}
