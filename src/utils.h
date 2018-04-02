#ifndef UTILS_H
#define UTILS_H

#include <QString>

QString readTextFile(const QString& path);
QByteArray fromSysexTextFile(const QString& path);
bool initFilesystem();
QString defaultDbPath();
void writeProgramFile(const QByteArray& sysex, const QString&);

#endif // UTILS_H
