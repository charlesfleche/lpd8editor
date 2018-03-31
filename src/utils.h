#ifndef UTILS_H
#define UTILS_H

#include <QString>

class QAbstractItemModel;

QString readTextFile(const QString& path);
QByteArray fromSysexTextFile(const QString& path);
bool initFilesystem();
QString defaultDbPath();
void writeProgramFile(const QByteArray& sysex, const QString&);
int getProgramId(const QAbstractItemModel*, int);

#endif // UTILS_H
