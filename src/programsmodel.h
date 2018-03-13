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

    QVariant data(const QModelIndex &idx, int role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value, int role) Q_DECL_OVERRIDE;

    Q_INVOKABLE int createProgram(const QString &name, const QByteArray &sysex);
    Q_INVOKABLE bool deleteProgram(int programId);

    Q_INVOKABLE QString name(int programId) const;
    Q_INVOKABLE QByteArray sysex(int programId) const;

private:
    int programRow(int programId) const;

    QSqlTableModel* m_pads;
    QSqlTableModel* m_knobs;
};

#endif // PROGRAMSMODEL_H
