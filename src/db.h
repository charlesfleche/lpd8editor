#ifndef DB_H
#define DB_H

#include <QtSql>

#include <expected>

int createProgram(const QString &programName = QString(),
                  const QByteArray &sysex = QByteArray(),
                  int programId = -1);
QString programName(int programId);
QByteArray programSysex(int programId);
bool fromSysex(int programId, const QByteArray &sysex);
QList<int> programIds();

QSqlError initDb(const QSqlDatabase &db);

// Opens the SQLite database at 'path' (creating its folder and the file if
// needed) as the given connection, and creates its schema. Returns the
// database, or a message for the user saying what went wrong.
std::expected<QSqlDatabase, QString>
openDatabase(const QString &path,
             QLatin1StringView connectionName = QLatin1StringView(QSqlDatabase::defaultConnection));
bool deleteProgram(int programId);

#endif // DB_H
