#ifndef MIDITRANSPORT_H
#define MIDITRANSPORT_H

#include <QByteArray>
#include <QByteArrayView>
#include <QFlags>
#include <QList>
#include <QObject>
#include <QString>

#include <optional>

namespace lpd8 {

// A MIDI port the application can connect to, as a transport lists it.
struct MidiPort
{
    // How the application's own port is connected to this one: From, it
    // receives what this port sends; To, it sends to this port.
    enum Direction { Disconnected = 0x00, From = 0x01, To = 0x02, BothWays = From | To };
    Q_DECLARE_FLAGS(Directions, Direction)

    // Opaque, and valid only until portsChanged() reports the port gone:
    // ALSA gives a replugged device a new client number (20, then 24). Never
    // persist it. A feature that remembers a device must use its name, and
    // look it up again after each portsChanged().
    QString id;
    QString name;
    Directions connection;

    bool operator==(const MidiPort &) const = default;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MidiPort::Directions)

// How the application sends and receives MIDI (doc/ARCHITECTURE.md,
// "Device"; doc/IMPROVEMENTS.md, item 4.8a). It carries complete SysEx
// messages, not backend events, so nothing above it depends on ALSA, and
// tests use a fake (tests/faketransport.h).
//
// The application has one port of its own. ports() lists the others, and
// what the application's port is connected to, both ways: connections made
// by other tools (aconnect, qjackctl) are listed too.
class MidiTransport : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(MidiTransport)

public:
    explicit MidiTransport(QObject *parent = nullptr);
    ~MidiTransport() override;

    // The ports, in the backend's order. The application's own port is not
    // listed.
    virtual QList<MidiPort> ports() const = 0;

    // Connects the application's port to 'portId', both ways, after
    // disconnecting it from everything else. False if there is no such port
    // or the backend refuses.
    virtual bool connectTo(const QString &portId) = 0;
    // Disconnects the application's port from every port.
    virtual void disconnectAll() = 0;

    // The port the application is connected to both ways, if it is the only
    // connection. Nothing if there is no connection, or if the connections
    // were made by another tool: one direction only, or several ports.
    std::optional<QString> connectedPort() const;
    // Any connection at all, whoever made it.
    bool isConnected() const;
    // Connected, but not as connectTo() would: another tool manages it.
    bool externallyManaged() const;

    // Sends one complete SysEx message: F0, data bytes (00 to 7F), F7.
    // Anything else is refused. It goes to whatever the application's port
    // sends to (its subscribers), including connections made by other
    // tools. False, and nothing sent, if the message is refused, if nothing
    // receives from the application's port, or if the backend fails.
    bool sendSysex(QByteArrayView message);

    // Whether 'message' is one complete SysEx message, as sendSysex() wants.
    static bool isCompleteSysex(QByteArrayView message);

signals:
    // A complete SysEx message, from the port 'sourcePortId'.
    void sysexReceived(const QByteArray &message, const QString &sourcePortId);
    // A port appeared, disappeared or was renamed.
    void portsChanged();
    // A connection of the application's port was made or broken, by the
    // application or by another tool.
    void connectionChanged();

protected:
    // Sends a message that sendSysex() checked.
    virtual bool sendCheckedSysex(QByteArrayView message) = 0;
};

} // namespace lpd8

#endif // MIDITRANSPORT_H
