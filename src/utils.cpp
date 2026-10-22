#include "utils.h"

#include "lpd8_sysex.h"

#include <QDir>
#include <QStandardPaths>
#include <QTextStream>

#include <QtDebug>

QString readTextFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QFile::ReadOnly | QFile::Text)) {
        QString msg("Failed to read file: ");
        msg += path;
        throw std::runtime_error(msg.toStdString());
    }
    QTextStream ts(&f);
    return ts.readAll();
}

namespace {

QDir dataDir()
{
    return {QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)};
}

} // namespace

bool initFilesystem()
{
    return dataDir().mkpath(".");
}

QString defaultDbPath()
{
    return dataDir().filePath("db.sqlite3");
}

char readAndValidateChar(QTextStream &in, char expected)
{
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

char readAndValidateChar(QTextStream &in, char min, char max)
{
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

char readAndValidateChar(QTextStream &in, const QList<char> &allowed)
{
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

QByteArray fromSysexTextFile(const QString &path)
{
    QByteArray ret;

    QFile data(path);
    if (!data.open(QFile::ReadOnly)) {
        qWarning() << "Cannot open sysex file" << path;
        return {};
    }

    QTextStream in(&data);
    for (int i = 0; i < sysex::programRequestSize(); ++i) {
        if (in.atEnd()) {
            qWarning() << "Reading from sysex file ended prematurely at position" << i;
            return {};
        }

        int c = 0;
        in >> c;
        const char ch = static_cast<char>(c);
        ret.append(ch);
    }

    return ret;
}

void writeProgramFile(const QByteArray &sysex, const QString &path)
{
    QFile f(path);
    if (!f.open(QFile::WriteOnly | QFile::Truncate)) {
        qWarning() << "Failed to open" << path;
        return;
    }

    QTextStream out(&f);

    for (const char byte : sysex) {
        out << static_cast<unsigned char>(byte) << " ";
    }
}
