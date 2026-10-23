#ifndef MIDINAMES_H
#define MIDINAMES_H

#include <QStringList>

// What the editors show for MIDI values, one entry per value: the delegate's
// spin boxes and the toggle button use these lists (MidiDataRole::MidiValues).
namespace lpd8::midinames {

// 128 entries: '36: C1', '37: C♯/D♭1'... (middle C is C3, note 60).
const QStringList &notes();
// 128 entries: '0' to '127'.
const QStringList &values();
// 16 entries: '1' to '16' (channels are stored as 0 to 15).
const QStringList &channels();
// 2 entries: 'Momentary', 'Toggle'.
const QStringList &toggleModes();

} // namespace lpd8::midinames

#endif // MIDINAMES_H
