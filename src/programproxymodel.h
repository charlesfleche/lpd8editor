#ifndef PROGRAMPROXYMODEL_H
#define PROGRAMPROXYMODEL_H

#include <QIdentityProxyModel>

class ProgramProxyModel : public QIdentityProxyModel
{
Q_OBJECT

public:
    ProgramProxyModel(QObject *parent = Q_NULLPTR);

    virtual QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const;

public slots:
    void setActiveProgramId(int);

private:
    int m_active_program_id;
};

#endif // PROGRAMPROXYMODEL_H
