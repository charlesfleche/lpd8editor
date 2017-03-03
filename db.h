#ifndef DB_H
#define DB_H

#include <QtSql>

class QString;

QSqlError initDb(const QString&);
bool addPreset(const QString&);
bool deletePreset(int);
bool setPresetName(int, const QString&);

#endif // DB_H
