#ifndef FAKETRANSPORT_H
#define FAKETRANSPORT_H

#include "miditransport.h"

#include <QByteArray>
#include <QList>
#include <QString>

#include <algorithm>
#include <functional>
#include <utility>

// A MidiTransport for tests, with no MIDI at all (doc/IMPROVEMENTS.md, item
// 4.8a). The test plays the rest of the system: it adds and removes ports
// (a device plugged in or out), connects them as another tool would, and
// injects received messages. What the application sends is recorded.
//
// It behaves like the ALSA backend where the tests depend on it: a port
// that disappears takes its connections with it, and one that comes back
// (with a new id, like a replugged device) is not connected again.
//
// No Q_OBJECT: it declares no signals or slots of its own, so it needs no
// meta-object, and this header needs no moc.
class FakeTransport : public lpd8::MidiTransport
{
public:
    using MidiPort = lpd8::MidiPort;

    QList<MidiPort> ports() const override { return m_ports; }

    bool connectTo(const QString &portId) override
    {
        const auto it = find(portId);
        if (it == m_ports.end()) {
            return false;
        }
        for (MidiPort &port : m_ports) {
            port.connection = MidiPort::Disconnected;
        }
        it->connection = MidiPort::BothWays;
        emit connectionChanged();
        return true;
    }

    void disconnectAll() override
    {
        for (MidiPort &port : m_ports) {
            port.connection = MidiPort::Disconnected;
        }
        emit connectionChanged();
    }

    // The test's side.

    void addPort(const QString &id, const QString &name)
    {
        m_ports.append(MidiPort{.id = id, .name = name, .connection = MidiPort::Disconnected});
        emit portsChanged();
    }

    void removePort(const QString &id)
    {
        const auto it = find(id);
        if (it == m_ports.end()) {
            return;
        }
        const bool wasConnected = it->connection != MidiPort::Disconnected;
        m_ports.erase(it);
        emit portsChanged();
        if (wasConnected) {
            emit connectionChanged();
        }
    }

    // A connection made by another tool (aconnect, qjackctl).
    void setConnection(const QString &id, MidiPort::Directions connection)
    {
        const auto it = find(id);
        if (it != m_ports.end()) {
            it->connection = connection;
            emit connectionChanged();
        }
    }

    // A complete message arriving from 'fromPort'.
    void receive(const QByteArray &message, const QString &fromPort)
    {
        emit sysexReceived(message, fromPort);
    }

    // Every message sent, in order.
    const QList<QByteArray> &sent() const { return m_sent; }

    // Called with each message sent, after it is recorded: how a simulated
    // device (tests/lpd8simulator.h) hears the application. There is one
    // hook, so one simulator per fake: setting a second one while the first
    // is there is a mistake (it would silently take the first one's
    // traffic), and asserts. An empty function removes it.
    void setSendHook(std::function<void(const QByteArray &)> hook)
    {
        Q_ASSERT(!hook || !m_sendHook);
        m_sendHook = std::move(hook);
    }

protected:
    bool sendCheckedSysex(QByteArrayView message) override
    {
        m_sent.append(message.toByteArray());
        if (m_sendHook) {
            m_sendHook(m_sent.last());
        }
        return true;
    }

private:
    QList<MidiPort>::iterator find(const QString &id)
    {
        return std::ranges::find(m_ports, id, &MidiPort::id);
    }

    QList<MidiPort> m_ports;
    QList<QByteArray> m_sent;
    std::function<void(const QByteArray &)> m_sendHook;
};

#endif // FAKETRANSPORT_H
