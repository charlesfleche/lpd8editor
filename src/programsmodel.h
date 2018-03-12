#ifndef PROGRAMSMODEL_H
#define PROGRAMSMODEL_H

#include <QSqlTableModel>

class QAbstractItemModel;
class QSqlTableModel;

class ProgramsModel : public QSqlTableModel
{
    Q_OBJECT

public:
    ProgramsModel(QObject* parent = Q_NULLPTR);

    Q_INVOKABLE int createProgram(const QString &name, const QByteArray &sysex);
    Q_INVOKABLE bool deleteProgram(int programId);

    Q_INVOKABLE QString name(int programId) const;
    Q_INVOKABLE QByteArray sysex(int programId) const;

private:
    int programRow(int programId) const;

    QSqlTableModel* m_pads;
    QSqlTableModel* m_knobs;
};

#endif // PROGRAMSMODEL_H
