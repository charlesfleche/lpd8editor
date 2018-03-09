#include "programsmodel.h"

#include <QSqlTableModel>

#include <QtDebug>

ProgramsModel::ProgramsModel(QObject *parent) :
    QAbstractItemModel(parent),
    m_programs(Q_NULLPTR)
{
    m_programs = new QSqlTableModel(this);
    m_programs->setTable("programs");
    m_programs->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_programs->select();

    qDebug() << m_programs->columnCount() << m_programs->rowCount();
}

int ProgramsModel::columnCount(const QModelIndex &parent) const {
    Q_CHECK_PTR(m_programs);

    return m_programs->columnCount(parent);
}

QVariant ProgramsModel::data(const QModelIndex &index, int role) const {
    Q_CHECK_PTR(m_programs);

    qDebug() << "data" << index << role;
    return m_programs->data(index, role);
}

Qt::ItemFlags ProgramsModel::flags(const QModelIndex &index) const {
    Q_CHECK_PTR(m_programs);

    qDebug() << "flags" << index;
    return m_programs->flags(index);
}

QVariant ProgramsModel::headerData(int section, Qt::Orientation orientation, int role) const {
    Q_CHECK_PTR(m_programs);

    qDebug() << "headerData" << section << orientation << role;
    return m_programs->headerData(section, orientation, role);
}

QModelIndex ProgramsModel::index(int row, int column, const QModelIndex &parent) const {
    Q_CHECK_PTR(m_programs);

    qDebug() << "index" << row << column << parent;
    return m_programs->index(row, column, parent);
}

QModelIndex ProgramsModel::parent(const QModelIndex &index) const {
    Q_CHECK_PTR(m_programs);

    qDebug() << "parent" << index;
    return QModelIndex();
//    return m_programs->parent(child);
}

int ProgramsModel::rowCount(const QModelIndex &parent) const {
    Q_CHECK_PTR(m_programs);

    qDebug() << "rowCount" << parent;
    return m_programs->rowCount(parent);
}
