#include "programproxymodel.h"

#include "utils.h"

#include <QFont>

ProgramProxyModel::ProgramProxyModel(QObject* parent) : QIdentityProxyModel(parent),
                                         m_active_program_id(-1)
{

}

void ProgramProxyModel::setActiveProgramId(int programId)
{
    //
    return;
    Q_CHECK_PTR(sourceModel());

    if (m_active_program_id == programId) {
        return;
    }
    int old_id = m_active_program_id;
    m_active_program_id = programId;

    for (int row = 0 ; row < sourceModel()->rowCount() ; ++row) {
        int id = getProgramId(sourceModel(), row);
        if (id == old_id || id == m_active_program_id) {
            const QModelIndex idx(sourceModel()->index(row, 1));
            emit dataChanged(idx, idx, QVector<int>({Qt::FontRole}));
        }
    }
}

QVariant ProgramProxyModel::data(const QModelIndex &proxyIndex, int role) const
{
    QVariant d = QIdentityProxyModel::data(proxyIndex, role);
    int programId = getProgramId(sourceModel(), proxyIndex.row());

    if (programId != m_active_program_id || proxyIndex.column() != 1 || role != Qt::FontRole) {
        return d;
    }

    QFont ret;
    ret.setBold(true);
    return ret;
}
