#include "programsmodel.h"

#include "lpd8_sysex.h"

#include <QSortFilterProxyModel>
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
#include <QSqlQuery>

#include <QtDebug>

static const int programIdColumn = 0;

ProgramsModel::ProgramsModel(QObject *parent) :
    QSqlTableModel(parent)
{
    setTable("programs");
    setEditStrategy(QSqlTableModel::OnFieldChange);
    select();

//    connect(this,
//            &ProgramsModel::primeInsert,
//            [=](int row, QSqlRecord &/*record*/) {
//        QSqlRecord rec = record(row);
////        const int id = rec.value("programId").toInt();
//        /*
//        const int id = record.value("programId").toInt();
//        */
//        qDebug() << "####"
//                 << row
//                 << rec.value("programId")
//                 << rec.value("name")
//                 << rec.value("channel")
//                 << "----";

//    });
}

//QSortFilterProxyModel* ProgramsModel::padsModel(int program_id) {
//    if (!m_pads_models.contains(program_id)) {
//        QSortFilterProxyModel* proxy = new QSortFilterProxyModel(this);
//        proxy->setSourceModel(m_pads_models);
//        proxy->setFilterKeyColumn(programIdColumn);
//        m_pads_models[program_id] =
//    }
//}


//int ProgramsModel::createProject(const QString &name, const QByteArray &sysex) {
//    QSqlRecord r = record();
//    r.setValue("name", name);
//    r.setValue("channel", sysex::channel(sysex));
//    if (!insertRecord(-1, r)) {
//         qWarning() << "Cannot create program:" << lastError().text();
//    }
//    const int ret = query().lastInsertId().toInt();
//    qDebug() << ret;
//    return ret;
//}

int ProgramsModel::createProject(const QString &name, const QByteArray &sysex) {
    QSqlField fname("name", QVariant::String);
    QSqlField fchannel("channel", QVariant::Int);

    fname.setValue(QVariant(name));
    fchannel.setValue(QVariant(sysex::channel(sysex)));

    QSqlRecord r;
    r.append(fname);
    r.append(fchannel);

    if (!insertRecord(-1, r)) {
         qWarning() << "Cannot create program:" << lastError().text();
    }
    const int ret = query().lastInsertId().toInt();
    qDebug() << ret;
    return ret;
}
