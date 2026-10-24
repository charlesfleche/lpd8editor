#include "utils.h"

#include "lpd8codec.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
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

QString fileNameFor(const QString &name)
{
    QString ret;
    for (const QChar c : name) {
        const bool unsafe = c == u'/' || c == u'\\' || c.category() == QChar::Other_Control
                || c.category() == QChar::Other_Format;
        ret += unsafe ? u'_' : c;
    }
    // '..' and hidden files: leading dots are dropped, and spaces around
    // them, until neither is left (' .bashrc', '. . .x'). Trimming only at
    // the end would leave the dot after a space in front.
    for (QString previous; previous != ret;) {
        previous = ret;
        ret = ret.trimmed();
        while (ret.startsWith(u'.')) {
            ret.remove(0, 1);
        }
    }
    return ret;
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

namespace {

bool isAsciiSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

bool isAsciiDigit(char c)
{
    return c >= '0' && c <= '9';
}

} // namespace

std::expected<QByteArray, QString> parseProgramText(QByteArrayView text, const QString &file)
{
    // Parsed by hand rather than with QTextStream, whose '>> int' also reads
    // hexadecimal (0x63), octal (010 is 8), binary and signs, and wraps
    // around on overflow: 4294967301 became a valid 5. The format has only
    // ever been decimal numbers, which is all that is accepted here.
    constexpr qsizetype max_digits = 3; // 255
    constexpr qsizetype expected = lpd8::program_message_size;

    QByteArray ret;
    qsizetype pos = 0;
    while (true) {
        while (pos < text.size() && isAsciiSpace(text[pos])) {
            ++pos;
        }
        if (pos == text.size()) {
            break;
        }
        // Only whitespace may follow the last value (exported files end
        // with a space). More values mean the file is not one program: two
        // programs back to back, or something else entirely.
        if (ret.size() == expected) {
            return std::unexpected(
                    tr("%1 has data after the %2 values of a program: it is not one LPD8 program.")
                            .arg(file, QString::number(expected)));
        }
        const qsizetype start = pos;
        int value = 0;
        while (pos < text.size() && isAsciiDigit(text[pos]) && pos - start < max_digits) {
            value = (value * 10) + (text[pos] - '0');
            ++pos;
        }
        const bool endOfWord = pos == text.size() || isAsciiSpace(text[pos]);
        if (pos == start || !endOfWord || value > 255) {
            // Positions count from 1, as a person counts the values of the file.
            return std::unexpected(tr("%1 has an invalid value at position %2.")
                                           .arg(file, QString::number(ret.size() + 1)));
        }
        ret.append(static_cast<char>(value));
    }

    if (ret.size() < expected) {
        return std::unexpected(
                tr("%1 ends too early: %2 values instead of %3.")
                        .arg(file, QString::number(ret.size()), QString::number(expected)));
    }
    return ret;
}

std::expected<QByteArray, QString> fromSysexTextFile(const QString &path)
{
    const QString file = QDir::toNativeSeparators(path);

    // Only a regular file. Opening a FIFO waits for a writer, forever, and a
    // device such as /dev/zero never ends: either would freeze the window.
    // isSequential() checks again after opening, in case the path changed.
    const QFileInfo info(path);
    if (info.exists() && !info.isFile()) {
        return std::unexpected(tr("%1 is not a file.").arg(file));
    }

    QFile data(path);
    if (!data.open(QFile::ReadOnly)) {
        return std::unexpected(tr("Cannot open %1:\n%2").arg(file, data.errorString()));
    }
    if (data.isSequential()) {
        return std::unexpected(tr("%1 is not a file.").arg(file));
    }

    const QByteArray text = data.read(max_program_file_size + 1);
    if (text.size() > max_program_file_size) {
        return std::unexpected(tr("%1 is too large to be an LPD8 program.").arg(file));
    }
    return parseProgramText(text, file);
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
