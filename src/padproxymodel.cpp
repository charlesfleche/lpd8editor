#include "padproxymodel.h"

#include "enums.h"

PadProxyModel::PadProxyModel(QObject *parent) : QIdentityProxyModel(parent)
{

}

QVariant PadProxyModel::data(const QModelIndex &proxyIndex, int role) const
{
    if (role == MidiDataRole::MidiValueType) {
        return proxyIndex.column() == 5 ? MidiType::ToggleType : MidiType::DefaultType;
    }

    return QIdentityProxyModel::data(proxyIndex, role);
}

Qt::ItemFlags PadProxyModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags ret = QIdentityProxyModel::flags(index);
    if (data(index, MidiDataRole::MidiValueType) == MidiType::ToggleType) {
        ret |= Qt::ItemIsUserCheckable;
    }
    return ret;
}
