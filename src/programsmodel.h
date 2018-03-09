#ifndef PROGRAMSMODEL_H
#define PROGRAMSMODEL_H

#include <QSqlTableModel>

class QSqlTableModel;

class ProgramsModel : public QSqlTableModel
{
    Q_OBJECT

public:
    ProgramsModel(QObject* parent = Q_NULLPTR);
};

#endif // PROGRAMSMODEL_H
