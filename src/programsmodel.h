#ifndef PROGRAMSMODEL_H
#define PROGRAMSMODEL_H

#include <QRegularExpression>
#include <QSqlTableModel>

class QAbstractItemModel;
class QItemSelectionModel;
class QSortFilterProxyModel;
class QSqlTableModel;
class QStandardItemModel;

class ProgramsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ProgramsModel(QObject *parent = nullptr, const QSqlDatabase &db = QSqlDatabase());

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QModelIndex
    index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    QModelIndex padsParentIndex(const QModelIndex &programIndex) const;
    QModelIndex knobsParentIndex(const QModelIndex &programIndex) const;

    QAbstractItemModel *padsHeaderModel() const;
    QAbstractItemModel *knobsHeaderModel() const;

    QModelIndex programIndex(int programId) const;
    QString programName(int programId) const;
    QByteArray programSysex(int programId) const;

    bool updateProgramFromSysex(int programId, const QByteArray &sysex);

    int createProgram(const QString &programName = QString(),
                      const QByteArray &sysex = QByteArray(),
                      int programId = -1);
    bool deleteProgram(int programId);

public slots:
    void select();

private:
    QAbstractItemModel *model(const QModelIndex &index);
    const QAbstractItemModel *model(const QModelIndex &index) const;
    const QAbstractItemModel *modelFromParent(const QModelIndex &parent) const;

    void addFilters(int programId);
    void removeFilters(int programId);

    bool hasLut(const QModelIndex &index) const;
    QStringList lut(const QModelIndex &index) const;

    QHash<int, QSortFilterProxyModel *> m_groups_proxies;
    QHash<int, QSortFilterProxyModel *> m_pads_proxies;
    QHash<int, QSortFilterProxyModel *> m_knobs_proxies;

    QStandardItemModel *m_groups{nullptr};
    QSqlTableModel *m_pads{nullptr};
    QSqlTableModel *m_knobs{nullptr};
    QSqlTableModel *m_programs{nullptr};
    QStandardItemModel *m_empty{nullptr};

    QStringList m_lut_default;
    QStringList m_lut_note;
    QStringList m_lut_channel;
    QStringList m_lut_toggle;
    QHash<QString, QStringList> m_luts;
};

class ProgramIdSelectionRestorer : public QObject
{
    Q_OBJECT

public:
    explicit ProgramIdSelectionRestorer(QItemSelectionModel *model, QObject *parent = nullptr);

public slots:
    void store();
    void restore();

private:
    int selectedProgramId() const;
    void selectProgramId(int programId);

    QItemSelectionModel *m_model;
    int m_program_id{-1}; // -1: no program, as everywhere else
};

#endif // PROGRAMSMODEL_H
