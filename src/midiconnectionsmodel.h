#ifndef MIDICONNECTIONSMODEL_H
#define MIDICONNECTIONSMODEL_H
class IOMidi;

#include <QAbstractListModel>

#include <alsa/asoundlib.h>

class IOMidi;

class MidiConnectionsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Direction {
        Disconnected = 0x00,
        From = 0x01,
        To = 0x02,
        BothWays = 0x03
    };
    Q_DECLARE_FLAGS(Directions, Direction)
    Q_FLAG(Directions)

    Q_PROPERTY(QModelIndex connectedPort READ connectedPort NOTIFY connectedPortChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool externallyManaged READ externallyManaged NOTIFY externallyManagedChanged)

    MidiConnectionsModel(IOMidi *io);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    QModelIndex connectedPort() const;
    bool connected() const;
    bool externallyManaged() const;

public slots:
    void connectPort(const QModelIndex &index);

    void handleMidiEvent(const snd_seq_event_t* ev);

signals:
    void connectedPortChanged(const QModelIndex&);
    void connectedChanged(bool);
    void externallyManagedChanged(bool);

private:
    void addAddressIfAccepted(const snd_seq_addr_t *addr);
    void removeAddressIfAccepted(const snd_seq_addr_t *addr);
    void notifyDataChangedForAddress(const snd_seq_addr_t *addr);

    void addAddress(const snd_seq_addr_t *addr);
    bool acceptAddress(const snd_seq_addr_t* addr) const;
    bool acceptClient(const snd_seq_client_info_t *info) const;
    bool acceptPort(const snd_seq_port_info_t *info) const;

    void disconnectAllPorts() const;
    void connectPort(const snd_seq_addr_t &addr);

    QString portName(const QModelIndex& index) const;
    Directions portConnection(const QModelIndex& index) const;
    Directions portConnection(const snd_seq_addr_t &addr) const;

    IOMidi *m_io;
    QList<snd_seq_addr_t> m_addrs;
};

#endif // MIDICONNECTIONSMODEL_H
