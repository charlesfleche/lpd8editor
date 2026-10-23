#ifndef LPD8DEVICE_H
#define LPD8DEVICE_H

#include "program.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

#include <chrono>
#include <optional>

namespace lpd8 {

class MidiTransport;

// The LPD8 protocol over a MidiTransport (doc/ARCHITECTURE.md, "Device";
// doc/IMPROVEMENTS.md, item 4.10a): asks the device for a program and
// matches the answer to the request, and writes programs into its slots.
//
// - One request at a time. Others wait in a queue, in order, sends included
//   (they have no answer, but nothing must be sent between a get and its
//   answer). Every request ends with exactly one signal: programReceived()
//   for a get that was answered, sendDone() for a send that went out, or
//   requestFailed().
// - Only the answer to the request in flight is accepted: a program dump
//   for the slot asked for, from a port the application receives from. The
//   connections are read at that moment, never remembered: a replugged
//   device has a new id. With the connection made in lpd8editor, that port
//   is the device. With connections made by another tool (aconnect,
//   qjackctl, a JACK bridge), it is any port patched into the application,
//   so such setups keep working: still far stricter than before 4.10b,
//   when a dump from anyone was applied. Everything else is ignored (dumps
//   from other clients, which was the security finding of the review;
//   dumps nobody asked for; the wrong slot), counted in ignoredMessages()
//   and logged under the 'lpd8.device' category, at debug level.
// - A get not answered within the timeout fails. A request whose turn comes
//   while nothing is connected fails at once, and so does one in flight
//   when nothing sends to the application any more (the device
//   unplugged). Requests go to whatever receives from the application's
//   port (MidiTransport::sendSysex()).
// - The signals may come before the call that caused them returns:
//   requestFailed() from inside requestProgram() when nothing is connected,
//   programReceived() from inside it if a backend answers during the send.
//   Connect before calling. Handlers must not block (no modal exec(), no
//   nested event loop): they can be called from inside the request loop,
//   and a nested loop would run timers and MIDI in the middle of it. Several
//   requestFailed() can come at once (an unplug with requests queued): a
//   window shows one message for them.
class Lpd8Device : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Lpd8Device)

public:
    // Far above what a USB device needs to answer, short enough that the
    // user isn't left waiting on a device that will never answer.
    static constexpr std::chrono::milliseconds default_reply_timeout{2000};

    // 'transport' must outlive the device.
    explicit Lpd8Device(MidiTransport &transport, QObject *parent = nullptr);
    ~Lpd8Device() override;

    // Asks for the program in 'slot' (1 to 4). Ends with programReceived()
    // or requestFailed().
    void requestProgram(int slot);
    // Writes 'program' into 'slot'. Ends with sendDone() or requestFailed().
    void sendProgram(int slot, const Program &program);

    // A request is in flight or waiting.
    bool isBusy() const;

    // For tests, which don't wait two seconds.
    void setReplyTimeout(std::chrono::milliseconds timeout);

    // Messages received and not accepted, since the device was created.
    int ignoredMessages() const { return m_ignored; }

signals:
    void programReceived(int slot, const lpd8::Program &program);
    void sendDone(int slot);
    // 'reason' is for the user.
    void requestFailed(int slot, const QString &reason);
    void busyChanged(bool busy);

private:
    struct Request
    {
        enum class Kind { Get, Send };
        Kind kind{Kind::Get};
        int slot{0};
        Program program; // for a send
        quint64 id{0}; // set when it starts
    };

    void enqueue(const Request &request);
    void startNext();
    void finishCurrent();
    void handleSysex(const QByteArray &message, const QString &source);
    void ignore(const QString &why);
    void timedOut();
    void checkConnection();
    void updateBusy();
    bool receivesFrom(const QString &portId) const;
    bool receivesFromAnyPort() const;

    MidiTransport &m_transport;
    QList<Request> m_queue;
    std::optional<Request> m_current;
    QTimer m_timer;
    quint64 m_lastId{0};
    int m_ignored{0};
    bool m_starting{false}; // startNext() is running
    bool m_busy{false}; // as last signalled
};

} // namespace lpd8

#endif // LPD8DEVICE_H
