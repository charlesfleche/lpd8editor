#include "utils.h"

#include "lpd8codec.h"

#include <QCoreApplication>
#include <QDir>
#include <QSaveFile>
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

QString defaultDbPath()
{
    return dataDir().filePath("db.sqlite3");
}

QString defaultStorePath()
{
    return dataDir().filePath("programs.json");
}

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("ProgramFile", text);
}

} // namespace

std::expected<QByteArray, QString> fromSysexTextFile(const QString &path)
{
    QByteArray ret;
    const QString file = QDir::toNativeSeparators(path);

    QFile data(path);
    if (!data.open(QFile::ReadOnly)) {
        return std::unexpected(tr("Cannot open %1:\n%2").arg(file, data.errorString()));
    }

    const auto tooShort = [&](int count) {
        return std::unexpected(tr("%1 ends too early: %2 values instead of %3.")
                                       .arg(file,
                                            QString::number(count),
                                            QString::number(lpd8::program_message_size)));
    };

    QTextStream in(&data);
    for (int i = 0; i < lpd8::program_message_size; ++i) {
        if (in.atEnd()) {
            return tooShort(i);
        }

        int c = 0;
        in >> c;
        // '>>' fails silently, leaving c at 0, so check the stream status:
        // ReadPastEnd if only whitespace was left, ReadCorruptData if the
        // next word is not a number.
        if (in.status() == QTextStream::ReadPastEnd) {
            return tooShort(i);
        }
        if (in.status() != QTextStream::Ok || c < 0 || c > 255) {
            // Positions count from 1, as a person counts the values of the file.
            return std::unexpected(tr("%1 has an invalid value at position %2.")
                                           .arg(file, QString::number(i + 1)));
        }
        ret.append(static_cast<char>(c));
    }

    // Only whitespace may follow (exported files end with a space). More
    // values mean the file is not one program: two programs back to back,
    // or something else entirely. Cutting it silently would import
    // something the user didn't choose.
    in.skipWhiteSpace();
    if (!in.atEnd()) {
        return std::unexpected(
                tr("%1 has data after the %2 values of a program: it is not one LPD8 program.")
                        .arg(file, QString::number(lpd8::program_message_size)));
    }

    return ret;
}

std::expected<void, QString> writeProgramFile(const QByteArray &sysex, const QString &path)
{
    // QSaveFile writes to a temporary file and renames it to 'path' only in
    // commit(), once everything was written. A failed export never leaves a
    // truncated file, and never destroys the file it was replacing.
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        return std::unexpected(
                tr("Cannot write %1:\n%2").arg(QDir::toNativeSeparators(path), f.errorString()));
    }

    QTextStream out(&f);
    for (const char byte : sysex) {
        out << static_cast<unsigned char>(byte) << " ";
    }
    out.flush();

    if (!f.commit()) {
        return std::unexpected(
                tr("Cannot write %1:\n%2").arg(QDir::toNativeSeparators(path), f.errorString()));
    }
    return {};
}
