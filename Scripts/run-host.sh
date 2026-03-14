#!/usr/bin/env bash
set -euo pipefail

# run-host.sh
# Runs the built host binary from the project's build output.
# Usage: ./run-host.sh [args...]

BINARY=./build/src/host/vt_host

if [[ ! -x "$BINARY" ]]; then
    echo "[run-host] Host binary not found or not executable: $BINARY"
    echo "[run-host] Build the project first (./Scripts/install-and-build.sh)."
    exit 1
fi

exec "$BINARY" "$@"
