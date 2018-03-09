#ifndef PROGRAMSMODEL_H
#define PROGRAMSMODEL_H

#include <QAbstractItemModel>

class QSqlTableModel;

class ProgramsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ProgramsModel(QObject* parent = Q_NULLPTR);

    int columnCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column, const QModelIndex &parent) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &child) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;

private:
    QSqlTableModel* m_programs;
};

#endif // PROGRAMSMODEL_H
