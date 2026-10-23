#include "midinames.h"

#include <QCoreApplication>

namespace lpd8::midinames {

const QStringList &notes()
{
    static const QStringList names = [] {
        const QStringList pitches = {
                "C", "C♯/D♭", "D", "D♯/E♭", "E", "F", "F♯/G♭", "G", "G♯/A♭", "A", "A♯/B♭", "B"};
        QStringList result;
        for (int i = 0; i <= 127; ++i) {
            const QString &pitch = pitches.at(i % pitches.size());
            const int octave = (i / static_cast<int>(pitches.size())) - 2;
            result << QStringLiteral("%1: %2%3")
                              .arg(QString::number(i), pitch, QString::number(octave));
        }
        return result;
    }();
    return names;
}

const QStringList &values()
{
    static const QStringList names = [] {
        QStringList result;
        for (int i = 0; i <= 127; ++i) {
            result << QString::number(i);
        }
        return result;
    }();
    return names;
}

const QStringList &channels()
{
    static const QStringList names = [] {
        QStringList result;
        for (int i = 0; i <= 15; ++i) {
            result << QString::number(i + 1);
        }
        return result;
    }();
    return names;
}

const QStringList &toggleModes()
{
    // Translated once, at first use, so it follows the language loaded at
    // startup.
    static const QStringList names = {QCoreApplication::translate("lpd8", "Momentary"),
                                      QCoreApplication::translate("lpd8", "Toggle")};
    return names;
}

} // namespace lpd8::midinames
