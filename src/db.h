#ifndef DB_H
#define DB_H

#include <QtSql>

int createProgram(const QString &name = QString(), const QByteArray &sysex = QByteArray());
QSqlError initDb(const QString&);
bool addProgram(const QString&, int&);
bool deleteProgram(int);
int nextProgramId(int);
bool setProgramName(int, const QString&);
bool isValidProgramId(int);

#endif // DB_H
