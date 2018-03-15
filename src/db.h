#ifndef DB_H
#define DB_H

#include <QtSql>

int createProgram(const QString &programName = QString(), const QByteArray &sysex = QByteArray());
QString programName(int programId);
QByteArray programSysex(int programId);
QList<int> programIds();

QSqlError initDb(const QString&);
bool addProgram(const QString&, int&);
bool deleteProgram(int);
int nextProgramId(int);
bool setProgramName(int, const QString&);
bool isValidProgramId(int);

#endif // DB_H
