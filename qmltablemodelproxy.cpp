#include "qmltablemodelproxy.h"

QmlTableModelProxy::QmlTableModelProxy(QObject *parent) : QIdentityProxyModel(parent)
{
    connect(this,
            SIGNAL(sourceModelChanged()),
            SLOT(refreshRoleNames()));
}

QVariant QmlTableModelProxy::data(const QModelIndex &proxyIndex, int role) const
{
    Q_ASSERT(sourceModel());

    if (role < Qt::UserRole) {
        return sourceModel()->data(mapToSource(proxyIndex), role);
    }

    Q_ASSERT(proxyIndex.column() == 0);

    int column = role - Qt::UserRole;
    QModelIndex sourceIndex = sourceModel()->index(proxyIndex.row(), column);
    return sourceModel()->data(sourceIndex);
}

QHash<int, QByteArray> QmlTableModelProxy::roleNames() const
{
    Q_ASSERT(sourceModel());

    return m_role_names;
}

void QmlTableModelProxy::refreshRoleNames()
{
    Q_ASSERT(sourceModel());

    m_role_names = sourceModel()->roleNames();
    for (int i = 0 ; i < sourceModel()->columnCount() ; ++i) {
        m_role_names[Qt::UserRole+i] = sourceModel()->headerData(i, Qt::Horizontal).toByteArray();
    }
}

bool QmlTableModelProxy::setData(const QModelIndex &index, const QVariant &value, int role) {
    Q_ASSERT(sourceModel());

    if (role < Qt::UserRole) {
        return sourceModel()->setData(index, value, role);
    }

    Q_ASSERT(index.column() == 0);

    int column = role - Qt::UserRole;
    QModelIndex sourceIndex = sourceModel()->index(index.row(), column);
    return sourceModel()->setData(sourceIndex, value);
}
