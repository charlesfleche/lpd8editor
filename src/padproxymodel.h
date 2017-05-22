#ifndef PADPROXYMODEL_H
#define PADPROXYMODEL_H

#include <QIdentityProxyModel>

class PadProxyModel : public QIdentityProxyModel
{
Q_OBJECT

public:
    PadProxyModel(QObject *parent = Q_NULLPTR);

    QVariant data(const QModelIndex &proxyIndex, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

#endif // PADPROXYMODEL_H
