#include "programsmodel.h"

ProgramsModel::ProgramsModel(QObject *parent) :
    QSqlTableModel(parent)
{
    setTable("programs");
    setEditStrategy(QSqlTableModel::OnFieldChange);
    select();
}
