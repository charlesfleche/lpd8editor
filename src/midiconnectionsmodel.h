#ifndef MIDICONNECTIONSMODEL_H
#define MIDICONNECTIONSMODEL_H

#include <QAbstractListModel>
#include <QStringList>

namespace lpd8 {
class MidiTransport;
}

// The MIDI ports for the connection combo box, over a MidiTransport. Row 0
// is the 'Disconnect' choice; the others are the transport's ports, in its
// order. Rows are inserted and removed as ports come and go, never reset,
// so the combo box keeps its current item.
//
// connectedPort() keeps the semantics of the ALSA-based model it replaces
// (item 4.8b): a single port connected both ways is "the connected port",
// whoever connected it. One made both ways by aconnect is not
// "externally managed". Only one-way or multiple connections are. Changing
// that would be a behaviour change, with an item of its own.
class MidiConnectionsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    Q_PROPERTY(QModelIndex connectedPort READ connectedPort NOTIFY connectedPortChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool externallyManaged READ externallyManaged NOTIFY externallyManagedChanged)

    // 'transport' must outlive the model.
    explicit MidiConnectionsModel(lpd8::MidiTransport &transport, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    // The connected port's row; row 0 ('Disconnect') when nothing is
    // connected; an invalid index when the connections are another tool's.
    QModelIndex connectedPort() const;
    bool connected() const;
    bool externallyManaged() const;

public slots:
    // Row 0 disconnects everything; another row connects to that port.
    void connectPort(const QModelIndex &idx);

signals:
    void connectedPortChanged(const QModelIndex &);
    void connectedChanged(bool);
    void externallyManagedChanged(bool);

private:
    void updatePorts();
    void notifyConnectionChanged();

    lpd8::MidiTransport &m_transport;
    // The port ids of rows 1 and up, as last read from the transport.
    QStringList m_ids;
};

#endif // MIDICONNECTIONSMODEL_H
