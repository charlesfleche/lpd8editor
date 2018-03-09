#ifndef PADPROXYMODEL_H
#define PADPROXYMODEL_H

#include <QIdentityProxyModel>

class PadProxyModel : public QIdentityProxyModel
{
Q_OBJECT

public:
    PadProxyModel(QObject *parent = Q_NULLPTR);

    QVariant data(const QModelIndex &proxyIndex, int role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
};

#endif // PADPROXYMODEL_H
