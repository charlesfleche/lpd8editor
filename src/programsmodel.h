#ifndef PROGRAMSMODEL_H
#define PROGRAMSMODEL_H

#include <QRegExp>
#include <QSqlTableModel>

class QAbstractItemModel;
class QSortFilterProxyModel;
class QSqlTableModel;
class QStandardItemModel;

class ProgramsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ProgramsModel(QObject* parent = Q_NULLPTR);

    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
//    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
//    bool hasChildren(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &child) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
//    bool setData(const QModelIndex &index, const QVariant &value, int role) Q_DECL_OVERRIDE;

//    bool canFetchMore(const QModelIndex &parent) const Q_DECL_OVERRIDE;
//    void fetchMore(const QModelIndex &parent) Q_DECL_OVERRIDE;

    Q_INVOKABLE int createProgram(const QString &name, const QByteArray &sysex);
    Q_INVOKABLE bool deleteProgram(int programId);

    Q_INVOKABLE QString name(int programId) const;
    Q_INVOKABLE QByteArray sysex(int programId) const;

protected:
//    QModelIndex createIndex(int row, int column, void *ptr = Q_NULLPTR) const;
//    QModelIndex createIndex(int row, int column, quintptr id) const;

private slots:
    void refresh();

private:
    int programRow(int programId) const;

    QAbstractItemModel* model(const QModelIndex &idx);
    const QAbstractItemModel* model(const QModelIndex &idx) const;
    const QAbstractItemModel* modelFromParent(const QModelIndex &parent) const;

    void addFilters(int programId);
    void removeFilters(int programId);

    QHash<int, QSortFilterProxyModel*> m_groups_proxies;
    QHash<int, QSortFilterProxyModel*> m_pads_proxies;
    QHash<int, QSortFilterProxyModel*> m_knobs_proxies;

    QStandardItemModel* m_groups;
    QSqlTableModel* m_pads;
    QSqlTableModel* m_knobs;
    QSqlTableModel* m_programs;
    QStandardItemModel* m_empty;
};

#endif // PROGRAMSMODEL_H
