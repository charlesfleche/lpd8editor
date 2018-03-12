#include "programsmodel.h"

#include "lpd8_sysex.h"

#include <QSortFilterProxyModel>
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
#include <QSqlQuery>

#include <QtDebug>

static const QString program_id_field_name("programId");
static const QString name_field_name("name");
static const QString midi_field_name("channel");

ProgramsModel::ProgramsModel(QObject *parent) :
    QSqlTableModel(parent)
{
    setTable("programs");
    setEditStrategy(QSqlTableModel::OnFieldChange);
    select();
}

int ProgramsModel::createProgram(const QString &name, const QByteArray &sysex) {
    QSqlRecord r(record());
    r.remove(r.indexOf(program_id_field_name));
    r.setValue(name_field_name, name);
    r.setValue(midi_field_name, sysex::channel(sysex));

    if (!insertRecord(-1, r)) {
         qWarning() << "Cannot create program:" << lastError().text();
    }

    return query().lastInsertId().toInt();
}
