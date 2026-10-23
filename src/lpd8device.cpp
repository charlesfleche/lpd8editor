#include "lpd8device.h"

#include "lpd8codec.h"
#include "miditransport.h"

#include <QLoggingCategory>

#include <QScopedValueRollback>
#include <algorithm>

namespace lpd8 {

namespace {

// Debug messages are off by default: QT_LOGGING_RULES=lpd8.device.debug=true
// shows them.
Q_LOGGING_CATEGORY(lcDevice, "lpd8.device", QtInfoMsg)

} // namespace

Lpd8Device::Lpd8Device(MidiTransport &transport, QObject *parent)
    : QObject(parent)
    , m_transport(transport)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(default_reply_timeout);
    connect(&m_timer, &QTimer::timeout, this, &Lpd8Device::timedOut);
    connect(&m_transport, &MidiTransport::sysexReceived, this, &Lpd8Device::handleSysex);
    connect(&m_transport, &MidiTransport::connectionChanged, this, &Lpd8Device::checkConnection);
    connect(&m_transport, &MidiTransport::portsChanged, this, &Lpd8Device::checkConnection);
}

Lpd8Device::~Lpd8Device() = default;

void Lpd8Device::setReplyTimeout(std::chrono::milliseconds timeout)
{
    m_timer.setInterval(timeout);
}

bool Lpd8Device::isBusy() const
{
    return m_current.has_value() || !m_queue.isEmpty();
}

void Lpd8Device::requestProgram(int slot)
{
    enqueue(Request{.kind = Request::Kind::Get, .slot = slot, .program = {}, .id = 0});
}

void Lpd8Device::sendProgram(int slot, const Program &program)
{
    enqueue(Request{.kind = Request::Kind::Send, .slot = slot, .program = program, .id = 0});
}

void Lpd8Device::enqueue(const Request &request)
{
    m_queue.append(request);
    startNext();
}

void Lpd8Device::startNext()
{
    // An answer can come inside sendSysex() (a backend may deliver
    // reentrantly) and end the request: its handler calls this again. The
    // nested call returns at once, and the loop below goes on with the next
    // request, so nothing is sent from inside a send.
    if (m_starting) {
        return;
    }
    const QScopedValueRollback guard(m_starting, true);

    while (!m_current && !m_queue.isEmpty()) {
        Request request = m_queue.takeFirst();
        if (!m_transport.isConnected()) {
            emit requestFailed(request.slot, tr("Not connected to an LPD8."));
            continue;
        }
        const auto message = request.kind == Request::Kind::Get
                ? encodeGetProgram(request.slot)
                : encodeSendProgram(request.program, request.slot);
        if (!message) {
            emit requestFailed(request.slot,
                               tr("Not a valid request for slot %1.").arg(request.slot));
            continue;
        }

        // The request is in flight, and its timer runs, *before* the message
        // goes: an answer that comes during the send finds it.
        request.id = ++m_lastId;
        m_current = request;
        if (request.kind == Request::Kind::Get) {
            m_timer.start();
        }
        const bool sent = m_transport.sendSysex(*message);
        if (!m_current || m_current->id != request.id) {
            continue; // answered during the send: already done
        }
        if (!sent) {
            finishCurrent();
            emit requestFailed(request.slot, tr("The request could not be sent to the LPD8."));
            continue;
        }
        if (request.kind == Request::Kind::Send) {
            finishCurrent(); // no answer to wait for
            emit sendDone(request.slot);
        }
    }
    updateBusy();
}

void Lpd8Device::finishCurrent()
{
    m_timer.stop();
    m_current.reset();
}

void Lpd8Device::ignore(const QString &why)
{
    ++m_ignored;
    qCDebug(lcDevice) << "Ignoring a SysEx message:" << why;
}

void Lpd8Device::handleSysex(const QByteArray &message, const QString &source)
{
    const auto decoded = decodeProgram(message);
    if (!decoded || decoded->kind != DecodedProgram::Kind::Dump) {
        ignore(QStringLiteral("not a program dump, from %1").arg(source));
        return;
    }
    // From a port the application receives from, as it is connected now,
    // not when the request was sent: a replugged device has a new id, and
    // the old one may name another client. With the connection made in
    // lpd8editor, that is the one device; with one made by another tool
    // (aconnect, qjackctl, a JACK bridge), any port patched into the
    // application, as such setups always worked.
    if (!receivesFrom(source)) {
        ignore(QStringLiteral("a dump from %1, which is not connected to the application")
                       .arg(source));
        return;
    }
    if (!m_current || m_current->kind != Request::Kind::Get) {
        ignore(QStringLiteral("a dump of slot %1 that nobody asked for").arg(decoded->slot));
        return;
    }
    if (decoded->slot != m_current->slot) {
        ignore(QStringLiteral("a dump of slot %1, while slot %2 was asked for")
                       .arg(QString::number(decoded->slot), QString::number(m_current->slot)));
        return;
    }

    const int slot = m_current->slot;
    finishCurrent();
    emit programReceived(slot, decoded->program);
    startNext();
}

void Lpd8Device::timedOut()
{
    if (!m_current) {
        return;
    }
    const int slot = m_current->slot;
    finishCurrent();
    emit requestFailed(slot,
                       tr("The LPD8 did not answer within %1 seconds.")
                               .arg(static_cast<double>(m_timer.interval()) / 1000.0));
    startNext();
}

bool Lpd8Device::receivesFrom(const QString &portId) const
{
    const QList<MidiPort> ports = m_transport.ports();
    return std::ranges::any_of(ports, [&portId](const MidiPort &port) {
        return port.id == portId && port.connection.testFlag(MidiPort::From);
    });
}

bool Lpd8Device::receivesFromAnyPort() const
{
    const QList<MidiPort> ports = m_transport.ports();
    return std::ranges::any_of(
            ports, [](const MidiPort &port) { return port.connection.testFlag(MidiPort::From); });
}

void Lpd8Device::checkConnection()
{
    if (!isBusy() || receivesFromAnyPort()) {
        return;
    }
    // Nothing sends to the application any more (the device unplugged, or
    // disconnected): no answer can come. Fail now rather than at the
    // timeout. Another port being added, a MIDI monitor for one, doesn't
    // matter.
    QList<int> failed;
    if (m_current) {
        failed.append(m_current->slot);
        finishCurrent();
    }
    for (const Request &request : std::as_const(m_queue)) {
        failed.append(request.slot);
    }
    m_queue.clear();
    for (const int slot : failed) {
        emit requestFailed(slot, tr("The LPD8 was disconnected."));
    }
    updateBusy();
}

void Lpd8Device::updateBusy()
{
    if (isBusy() != m_busy) {
        m_busy = isBusy();
        emit busyChanged(m_busy);
    }
}

} // namespace lpd8
