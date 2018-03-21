#ifndef DB_H
#define DB_H

#include <QtSql>

int createProgram(const QString &programName = QString(), const QByteArray &sysex = QByteArray(), int programId = -1);
QString programName(int programId);
QByteArray programSysex(int programId);
QList<int> programIds();
int programChannel(int programId);
bool setProgramChannel(int programId, int channel);

QSqlError initDb(const QString&);
bool deleteProgram(int);
bool isValidProgramId(int);

#endif // DB_H
