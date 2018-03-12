#ifndef UTILS_H
#define UTILS_H

#include "lpd8_types.h"

#include <QString>

class QAbstractItemModel;

QString readTextFile(const QString& path);
QByteArray fromSysexTextFile(const QString& path);
bool initFilesystem();
QString defaultDbPath();
pProgram readProgramFile(const QString&);
void writeProgramFile(pProgram, const QString&);
int getProgramId(const QAbstractItemModel*, int);

#endif // UTILS_H
