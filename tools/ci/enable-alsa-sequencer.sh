#!/bin/sh
# Makes the ALSA sequencer (/dev/snd/seq) available on a CI machine, so
# that the tests which open it run instead of skipping. They are those of
# the ALSA backend (alsaseqtransport_test), among them the tests of what
# another local client can do to lpd8editor.
#
# GitHub's runners have no sound card, but the sequencer is a kernel module
# of its own (snd-seq), which needs no hardware. On Ubuntu's cloud kernels
# it ships in linux-modules-extra. Containers can't load modules, so run
# this on the runner itself.
#
# Use with LPD8EDITOR_REQUIRE_ALSA=1 in the test environment, so that a
# missing sequencer fails the tests instead of skipping them silently.

set -eu

sudo=""
if [ "$(id -u)" -ne 0 ]; then
    sudo="sudo"
fi

if [ ! -e /dev/snd/seq ]; then
    if ! $sudo modprobe snd-seq 2>/dev/null; then
        export DEBIAN_FRONTEND=noninteractive
        # A fresh runner's package lists may not know this kernel's package.
        $sudo apt-get update
        $sudo apt-get install --yes --no-install-recommends "linux-modules-extra-$(uname -r)"
        $sudo modprobe snd-seq
    fi
fi

# The device belongs to the 'audio' group; the runner's user is not in it.
$sudo chmod a+rw /dev/snd/seq

if [ ! -r /dev/snd/seq ] || [ ! -w /dev/snd/seq ]; then
    echo "The ALSA sequencer is still not available" >&2
    exit 1
fi
echo "ALSA sequencer available: $(ls -l /dev/snd/seq)"
