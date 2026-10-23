#ifndef UTILS_H
#define UTILS_H

#include <QString>

#include <expected>

QString readTextFile(const QString &path);
// Program files hold the bytes of a program message as decimal numbers
// separated by spaces. Both functions return a message for the user on
// failure.
std::expected<QByteArray, QString> fromSysexTextFile(const QString &path);
QString defaultDbPath();
// The programs file of ProgramStore, next to the old database.
QString defaultStorePath();
std::expected<void, QString> writeProgramFile(const QByteArray &sysex, const QString &path);

#endif // UTILS_H
