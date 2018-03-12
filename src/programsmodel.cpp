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
         return -1;
    }

    return query().lastInsertId().toInt();
}

bool ProgramsModel::deleteProgram(int programId) {
    QModelIndexList indices(match(index(0, 0), Qt::DisplayRole, programId));
    Q_ASSERT(indices.count() == 1);

    const bool ret = removeRow(indices[0].row());
    if (ret) {
        select();
    } else {
        qWarning() << "Failed to delete program" << programId;
    }
    return ret;
}
