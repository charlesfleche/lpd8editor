#ifndef DB_H
#define DB_H

#include <QtSql>

int createProgram(const QString &programName = QString(), const QByteArray &sysex = QByteArray(), int programId = -1);
QString programName(int programId);
QByteArray programSysex(int programId);
bool fromSysex(int programId, const QByteArray &sysex);
QList<int> programIds();

QSqlError initDb(const QString&);
bool deleteProgram(int);

#endif // DB_H
