#include "lpd8controller.h"

#include "midiio.h"

Lpd8Controller::Lpd8Controller(QObject *parent) : QObject(parent)
{
    MidiIO* io = new MidiIO(this);
    connect(io,
            &MidiIO::eventReceived,
            this,
            &Lpd8Controller::processEvent);
}

void Lpd8Controller::processEvent(snd_seq_event_t* ev) {
    switch (ev->type) {
    case SND_SEQ_EVENT_CLIENT_START:
        processClientStart(ev);
        break;
    default:
        break;
    }
}

void Lpd8Controller::processClientStart(snd_seq_event_t* ev) {
    Q_CHECK_PTR(ev);
    Q_ASSERT(ev->type);
}
