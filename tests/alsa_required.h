#ifndef ALSA_REQUIRED_H
#define ALSA_REQUIRED_H

#include <QtGlobal>

// Whether the tests that open the ALSA sequencer must fail, rather than
// skip, when there is none. A machine without /dev/snd/seq skips them; CI
// sets LPD8EDITOR_REQUIRE_ALSA=1 where it made the sequencer available
// (tools/ci/enable-alsa-sequencer.sh), so that a setup that breaks shows
// up as a failure instead of silently testing nothing. Among these tests are
// those of what another local client can do to lpd8editor
// (alsaseqtransport_test).
inline bool alsaRequired()
{
    return qEnvironmentVariableIsSet("LPD8EDITOR_REQUIRE_ALSA");
}

#endif // ALSA_REQUIRED_H
