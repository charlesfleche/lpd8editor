#ifndef DB_H
#define DB_H

#include <QtSql>

class QString;

QSqlError initDb(const QString&);

#endif // DB_H
