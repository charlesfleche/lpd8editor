#ifndef UTILS_H
#define UTILS_H

#include <QByteArrayView>
#include <QString>

#include <expected>

QString readTextFile(const QString &path);
// Program files hold the bytes of a program message as decimal numbers
// separated by spaces. Both functions return a message for the user on
// failure.
std::expected<QByteArray, QString> fromSysexTextFile(const QString &path);
// The parser behind fromSysexTextFile(), on the contents of a file: exactly
// one program's worth of decimal numbers from 0 to 255 (1 to 3 digits each),
// separated by ASCII whitespace, and nothing else. 'file' names the file in
// the error message.
std::expected<QByteArray, QString> parseProgramText(QByteArrayView text, const QString &file);
// Program files larger than this are refused without being read: 66 values
// take at most 264 bytes, and an exported file 264.
inline constexpr qint64 max_program_file_size = qint64{64} * 1024;
QString defaultDbPath();
// A file name made from 'name' (a program name, which the user, an imported
// file or an old database chose) that stays in the folder it is put in: no
// path separator, no leading dot (no '..', no hidden file), no control or
// format character. Empty if nothing is left.
QString fileNameFor(const QString &name);
// The programs file of ProgramStore, next to the old database.
QString defaultStorePath();
std::expected<void, QString> writeProgramFile(const QByteArray &sysex, const QString &path);

#endif // UTILS_H
