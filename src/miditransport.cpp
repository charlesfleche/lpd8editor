#include "miditransport.h"

#include <algorithm>

namespace lpd8 {

namespace {

constexpr char sysex_start = '\xf0';
constexpr char sysex_end = '\xf7';
constexpr unsigned char data_byte_max = 0x7f;

} // namespace

MidiTransport::MidiTransport(QObject *parent)
    : QObject(parent)
{ }

MidiTransport::~MidiTransport() = default;

std::optional<QString> MidiTransport::connectedPort() const
{
    // The rules of MidiConnectionsModel::connectedPort() until item 4.8b:
    // one port connected both ways, and nothing else.
    std::optional<QString> found;
    for (const MidiPort &port : ports()) {
        if (port.connection == MidiPort::Disconnected) {
            continue;
        }
        if (port.connection != MidiPort::BothWays || found) {
            return std::nullopt; // one way only, or a second port
        }
        found = port.id;
    }
    return found;
}

bool MidiTransport::isConnected() const
{
    const QList<MidiPort> all = ports();
    return std::ranges::any_of(
            all, [](const MidiPort &port) { return port.connection != MidiPort::Disconnected; });
}

bool MidiTransport::externallyManaged() const
{
    return isConnected() && !connectedPort();
}

bool MidiTransport::isCompleteSysex(QByteArrayView message)
{
    if (message.size() < 2 || message.front() != sysex_start || message.back() != sysex_end) {
        return false;
    }
    const QByteArrayView data = message.sliced(1, message.size() - 2);
    return std::ranges::all_of(
            data, [](char c) { return static_cast<unsigned char>(c) <= data_byte_max; });
}

bool MidiTransport::sendSysex(QByteArrayView message)
{
    if (!isCompleteSysex(message)) {
        return false;
    }
    const QList<MidiPort> all = ports();
    const bool someoneListens = std::ranges::any_of(
            all, [](const MidiPort &port) { return port.connection.testFlag(MidiPort::To); });
    if (!someoneListens) {
        return false;
    }
    return sendCheckedSysex(message);
}

} // namespace lpd8
