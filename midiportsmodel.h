#ifndef MIDIPORTSMODEL_H
#define MIDIPORTSMODEL_H

#include <QAbstractListModel>

#include <asoundlib.h>

class MidiPortsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit MidiPortsModel( snd_seq_t* seq_handle, int, QObject *parent = Q_NULLPTR);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Add data:
//    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    // Remove data:
//    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

public slots:
    void rescanPorts();

    void connect(const QModelIndex&) const;

private:
    QList<snd_seq_addr_t> m_port_addrs;
    snd_seq_t* m_seq_handle;
    int m_seq_port;
};

#endif // MIDIPORTSMODEL_H
