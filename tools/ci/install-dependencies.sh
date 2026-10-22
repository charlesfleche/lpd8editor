#!/bin/sh
# Install the build dependencies on Debian or Ubuntu.
#
#     tools/ci/install-dependencies.sh [extra packages...]
#
# Used by the GitHub workflows, both in the Debian trixie container (where
# we are root) and on the Ubuntu runner (where we need sudo). Extra
# packages, such as the analysis tools, are passed as arguments.

set -eu

packages="
    ca-certificates
    git
    cmake
    ninja-build
    g++
    clang
    libasound2-dev
    qt6-base-dev
    qt6-svg-dev
"

sudo=""
if [ "$(id -u)" -ne 0 ]; then
    sudo="sudo"
fi

# Never stop to ask a question (tzdata and friends).
export DEBIAN_FRONTEND=noninteractive

$sudo apt-get update
# shellcheck disable=SC2086 # the lists are meant to be split into words
$sudo apt-get install --yes --no-install-recommends $packages "$@"
