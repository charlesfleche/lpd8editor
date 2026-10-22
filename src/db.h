#ifndef DB_H
#define DB_H

#include <QtSql>

#include <expected>

// Limitation: the functions below run on the *default* connection
// (QSqlDatabase::database()), whatever was passed to openDatabase(),
// ProgramsModel or MainWindow. Those assert that they got the default
// connection. db.cpp is replaced by the JSON store (doc/IMPROVEMENTS.md,
// item 4.7), so the connection is not threaded through here.

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
//
// Only the default connection works with the rest of this file (see the
// limitation above). 'connectionName' exists so the tests of this function
// can open a second database without replacing the default connection.
std::expected<QSqlDatabase, QString>
openDatabase(const QString &path,
             QLatin1StringView connectionName = QLatin1StringView(QSqlDatabase::defaultConnection));
bool deleteProgram(int programId);

#endif // DB_H
