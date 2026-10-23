#ifndef LPD8SIMULATOR_H
#define LPD8SIMULATOR_H

#include "deviceprofile.h"
#include "faketransport.h"
#include "lpd8codec.h"
#include "program.h"

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

#include <array>

// An LPD8 behind a FakeTransport port, for tests (doc/IMPROVEMENTS.md, item
// 4.9). It answers like the device, as doc/SYSEX.md describes it, and only
// that:
//
// - Four slots, each holding a program, the factory program at first.
// - 'Get program' (63 00 01 0X F7, X from 1 to 4) is answered with the
//   program dump of slot X (63 00 3A 0X, then the program, 66 bytes).
// - 'Send program' (61 00 3A 0X, then the program, 66 bytes) stores it in
//   slot X. There is no answer, as on the device.
// - It hears what the application sends only while the application sends
//   to its port, and its answers reach the application only while the
//   application receives from it, as with ALSA subscriptions. The source of
//   an answer is the simulator's port.
//
// Answers never come inside the application's sendSysex() call: a device
// can't answer before the request has left. Each comes on a later turn of
// the event loop, one per turn, in the order of the requests. A
// synchronous answer would let through code that only works by reentrancy.
//
// What it does NOT simulate, on purpose:
// - Timing: no latency, no USB pacing. A test waits for an event-loop turn.
// - What the device does with anything else: a malformed request, a slot
//   outside 1 to 4, a wrong size, a value above 7F, or messages it has and
//   the application doesn't use (get or set the active program, device
//   info). None of this is documented, so the simulator ignores it, without
//   guessing, and counts it: a test can check that the application sent
//   nothing unexpected.
// - Several requests in flight: it answers in order, and has no mode that
//   answers out of order. The application sends one request at a time, and
//   Lpd8Device (item 4.10) enforces that.
// - Unsolicited dumps from other clients: a test sends them with
//   FakeTransport::receive(), from another port.
class Lpd8Simulator
{
public:
    // Answer, or not, to probe the application's error handling.
    enum class Mode {
        Normal,
        Silent, // hears and stores, but never answers
        WrongSlot, // answers a get with the dump of the next slot
    };

    // Plugs the simulated device into 'transport', as the port 'portId',
    // named "LPD8". 'transport' must outlive the simulator.
    explicit Lpd8Simulator(FakeTransport &transport, QString portId = QStringLiteral("20:0"))
        : m_transport(transport)
        , m_portId(std::move(portId))
    {
        m_slots.fill(lpd8::profile().defaultProgram);
        m_transport.addPort(m_portId, QStringLiteral("LPD8"));
        m_transport.setSendHook([this](const QByteArray &message) { hear(message); });
    }

    // Destroying the simulator unplugs the device: its port goes, with
    // portsChanged() and, if it was connected, connectionChanged(), as on a
    // real unplug. An answer still pending is lost.
    ~Lpd8Simulator()
    {
        m_transport.setSendHook({});
        m_transport.removePort(m_portId);
    }

    Lpd8Simulator(const Lpd8Simulator &) = delete;
    Lpd8Simulator &operator=(const Lpd8Simulator &) = delete;
    Lpd8Simulator(Lpd8Simulator &&) = delete;
    Lpd8Simulator &operator=(Lpd8Simulator &&) = delete;

    const QString &portId() const { return m_portId; }

    // Slots count from 1, as on the device.
    const lpd8::Program &program(int slot) const { return m_slots.at(index(slot)); }
    void setProgram(int slot, const lpd8::Program &program) { m_slots.at(index(slot)) = program; }

    void setMode(Mode mode) { m_mode = mode; }

    // The requests it recognized (gets and sends), and the other messages.
    int requests() const { return m_requests; }
    int unrecognized() const { return m_unrecognized; }
    // Answers not delivered yet.
    qsizetype pendingAnswers() const { return m_pending.size(); }

private:
    static constexpr qsizetype get_size = 9;
    static constexpr qsizetype program_size = 66;
    static constexpr qsizetype slot_offset = 7;

    static std::size_t index(int slot)
    {
        Q_ASSERT(slot >= 1 && slot <= 4);
        return static_cast<std::size_t>(slot - 1);
    }

    lpd8::MidiPort::Directions connection() const
    {
        const QList<lpd8::MidiPort> ports = m_transport.ports();
        const auto it = std::ranges::find(ports, m_portId, &lpd8::MidiPort::id);
        return it != ports.end() ? it->connection
                                 : lpd8::MidiPort::Directions(lpd8::MidiPort::Disconnected);
    }

    static bool startsWith(const QByteArray &message, std::initializer_list<int> bytes)
    {
        qsizetype i = 0;
        for (const int b : bytes) {
            if (i >= message.size() || static_cast<unsigned char>(message.at(i)) != b) {
                return false;
            }
            ++i;
        }
        return true;
    }

    void hear(const QByteArray &message)
    {
        if (!connection().testFlag(lpd8::MidiPort::To)) {
            return; // sent to others, not to this device
        }
        const int slot = message.size() > slot_offset
                ? static_cast<unsigned char>(message.at(slot_offset))
                : 0;
        const bool validSlot = slot >= 1 && slot <= 4;

        if (validSlot && message.size() == get_size
            && startsWith(message, {0xf0, 0x47, 0x7f, 0x75, 0x63, 0x00, 0x01})) {
            ++m_requests;
            if (m_mode != Mode::Silent) {
                answer(m_mode == Mode::WrongSlot ? (slot % 4) + 1 : slot);
            }
            return;
        }
        if (validSlot && message.size() == program_size
            && startsWith(message, {0xf0, 0x47, 0x7f, 0x75, 0x61, 0x00, 0x3a})) {
            const auto decoded = lpd8::decodeProgram(message);
            if (decoded) {
                ++m_requests;
                m_slots.at(index(slot)) = decoded->program;
                return;
            }
        }
        ++m_unrecognized;
    }

    // Queues the dump of 'slot', delivered on a later event-loop turn.
    void answer(int slot)
    {
        QByteArray dump = lpd8::encodeStoredProgram(program(slot));
        dump[6] = '\x3a'; // 63 00 3A: program dump
        dump[slot_offset] = static_cast<char>(slot);
        m_pending.append(dump);
        if (m_pending.size() == 1) {
            scheduleDelivery();
        }
    }

    void scheduleDelivery()
    {
        // m_context: a pending delivery dies with the simulator.
        QTimer::singleShot(0, &m_context, [this] {
            const QByteArray dump = m_pending.takeFirst();
            if (connection().testFlag(lpd8::MidiPort::From)) {
                m_transport.receive(dump, m_portId);
            }
            if (!m_pending.isEmpty()) {
                scheduleDelivery(); // one answer per turn
            }
        });
    }

    FakeTransport &m_transport;
    QString m_portId;
    std::array<lpd8::Program, 4> m_slots;
    Mode m_mode{Mode::Normal};
    int m_requests{0};
    int m_unrecognized{0};
    QList<QByteArray> m_pending;
    QObject m_context;
};

#endif // LPD8SIMULATOR_H
