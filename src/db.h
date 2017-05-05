#ifndef DB_H
#define DB_H

#include <QtSql>

class QString;

QSqlError initDb(const QString&);
bool addProgram(const QString&, int&);
bool deleteProgram(int);
bool setProgramName(int, const QString&);
bool isValidProgramId(int);

#endif // DB_H
