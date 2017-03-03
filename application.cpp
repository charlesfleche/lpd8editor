#include "application.h"

#include "db.h"
#include "utils.h"

#include <QSqlTableModel>

#include <exception>

Application::Application(QObject *parent):
    QObject(parent),
    m_presets(Q_NULLPTR)
{
    if (!initFilesystem()) {
        throw std::exception();
    }
    if (initDb(defaultDbPath()).isValid()) {
        throw std::exception();
    }

    QSqlTableModel* presets = new QSqlTableModel(this);
    presets->setTable("presets");
    presets->setEditStrategy(QSqlTableModel::OnFieldChange);
    presets->select();
    m_presets = presets;
}

QAbstractItemModel* Application::presets() const {
    return m_presets;
}
