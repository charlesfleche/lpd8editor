#ifndef UTILS_H
#define UTILS_H

#include "lpd8_types.h"

#include <QString>

bool initFilesystem();
QString defaultDbPath();
pProgram readProgramFile(const QString&);

#endif // UTILS_H
