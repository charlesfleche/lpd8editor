#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>

class ProgramsModel;

class QAbstractItemModel;
class QSqlTableModel;

class Application : public QObject
{
    Q_OBJECT

public:
    explicit Application(QObject *parent = 0);

    QAbstractItemModel* programs() const;
    ProgramsModel* myPrograms() const;

private:
    ProgramsModel* m_my_programs;
};

#endif // APPLICATION_H
