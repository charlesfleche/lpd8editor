#ifndef ALSASEQTRANSPORT_H
#define ALSASEQTRANSPORT_H

#include "miditransport.h"

#include <memory>

namespace lpd8 {

// MidiTransport over the ALSA sequencer (doc/IMPROVEMENTS.md, item 4.8b).
// PipeWire bridges ALSA sequencer clients, and JACK sees them through
// pipewire-jack or a2jmidid, so this one backend reaches the usual Linux
// setups.
//
// It opens one sequencer client, named after the application, with one
// port, and listens to the system announcements for hotplug. Port ids are
// "client:port", ALSA's numbers. They change when a device is replugged.
// ports() lists the ports of the other clients that can both send and
// receive, and are not hidden (SND_SEQ_PORT_CAP_NO_EXPORT).
//
// asoundlib.h is only included by the .cpp file (the Private class): no
// header of the application depends on ALSA.
class AlsaSeqTransport : public MidiTransport
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(AlsaSeqTransport)

public:
    // The largest message sendSysex() sends: what SysexAssembler accepts on
    // the other side, far above an LPD8 message (66 bytes). ALSA itself
    // allows more (see the .cpp file).
    static constexpr qsizetype max_sysex_size = 4096;

    // Opens the client. Throws std::runtime_error if there is no sequencer
    // (no /dev/snd/seq, as on most CI machines) or it can't be set up.
    explicit AlsaSeqTransport(QObject *parent = nullptr);
    ~AlsaSeqTransport() override;

    QList<MidiPort> ports() const override;
    bool connectTo(const QString &portId) override;
    void disconnectAll() override;

    // The id other transports list this one's port under.
    QString ownPortId() const;

    // At most this many sources have an unfinished SysEx message kept for
    // reassembly at a time. A source is a port id, and the port of an event
    // is the sender's choice: without a cap, one client could pose as 256
    // ports, each holding an unfinished message (see the .cpp file).
    static constexpr qsizetype max_unfinished_sources = 16;
    // The bytes of unfinished messages held now, for every source.
    qsizetype bufferedSysexBytes() const;

protected:
    bool sendCheckedSysex(QByteArrayView message) override;

private:
    void readEvents();

    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace lpd8

#endif // ALSASEQTRANSPORT_H
