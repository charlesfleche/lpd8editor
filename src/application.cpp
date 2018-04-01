#include "application.h"

#include "db.h"
#include "programsmodel.h"
#include "utils.h"

#include <QFileInfo>
#include <QSettings>
#include <QSqlTableModel>
#include <QtDebug>

#include <exception>

static const QString SETTINGS_KEY_ACTIVE_PROGRAM_ID = "active/program_id";

Application::Application(QObject *parent):
    QObject(parent),
    m_my_programs(Q_NULLPTR)
{
    if (!initFilesystem()) {
        throw std::runtime_error("Failed filesystem initialization");
    }
    if (initDb(defaultDbPath()).isValid()) {
        throw std::runtime_error("Failed database initialization");
    }

    m_my_programs = new ProgramsModel(this);
}

QAbstractItemModel* Application::programs() const {
    return m_my_programs;
}

ProgramsModel* Application::myPrograms() const {
    return m_my_programs;
}
