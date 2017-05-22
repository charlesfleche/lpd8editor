#include "padproxymodel.h"

PadProxyModel::PadProxyModel(QObject *parent) : QIdentityProxyModel(parent)
{

}

QVariant PadProxyModel::data(const QModelIndex &proxyIndex, int role) const
{
    QVariant ret = QIdentityProxyModel::data(proxyIndex, role);
    if (proxyIndex.column() != 5) { // 5 is the toggle column
        return ret;
    }

    const bool toggle = QIdentityProxyModel::data(proxyIndex, Qt::DisplayRole).toBool();
    switch (role) {
    case Qt::DisplayRole:
        ret = QVariant();
        break;
    case Qt::CheckStateRole:
        ret = toggle ? Qt::Checked : Qt::Unchecked;
        break;
    default:
        break;
    }
    return ret;
}

bool PadProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QVariant newValue = value;

    if (index.column() == 5) {
        newValue = newValue.toInt() > 0 ? 1 : 0;
        role = role == Qt::CheckStateRole ? Qt::EditRole : role;
    }

    return QIdentityProxyModel::setData(index, newValue, role);
}

Qt::ItemFlags PadProxyModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags ret = QIdentityProxyModel::flags(index);
    if (index.column() == 5) { // 5 is the toggle column
        ret &= ~Qt::ItemIsEditable;
        ret |= Qt::ItemIsUserCheckable;
    }
    return ret;
}
