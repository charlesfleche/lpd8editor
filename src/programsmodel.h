#ifndef PROGRAMSMODEL_H
#define PROGRAMSMODEL_H

#include <QSqlTableModel>

class QAbstractItemModel;
class QSqlTableModel;

class ProgramsModel : public QSqlTableModel
{
    Q_OBJECT

public:
    ProgramsModel(QObject* parent = Q_NULLPTR);

    Q_INVOKABLE int createProgram(const QString &name, const QByteArray &sysex);
    Q_INVOKABLE bool deleteProgram(int programId);

private:
//    QSortFilterProxyModel* padsModel(int program_id);
//    QSortFilterProxyModel* knobsModel(int program_id);

//    QHash<int, QSortFilterProxyModel*> m_pads_models;
//    QHash<int, QSortFilterProxyModel*> m_knobs_models;
};

#endif // PROGRAMSMODEL_H
