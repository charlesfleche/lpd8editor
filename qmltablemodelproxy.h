#ifndef QMLTABLEMODELPROXY_H
#define QMLTABLEMODELPROXY_H

#include <QIdentityProxyModel>

class QmlTableModelProxy : public QIdentityProxyModel
{
Q_OBJECT

public:
    QmlTableModelProxy(QObject *parent = Q_NULLPTR);

    virtual QVariant data(const QModelIndex &proxyIndex, int role) const;
    virtual QHash<int, QByteArray> roleNames() const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

private slots:
    void refreshRoleNames();

private:
    QHash<int, QByteArray> m_role_names;
};

#endif // QMLTABLEMODELPROXY_H
