#!/usr/bin/env bash
set -euo pipefail

# run-host.sh
# Runs the built host binary from the project's build output.
# Usage: ./run-host.sh [args...]

# Resolve project root (one level above the Scripts directory)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." >/dev/null 2>&1 && pwd)"

BINARY="$PROJECT_ROOT/build/src/host/vt_host"

echo "[run-host] Resolved project root: $PROJECT_ROOT"
echo "[run-host] Looking for host binary at: $BINARY"

if [[ ! -x "$BINARY" ]]; then
    echo "[run-host] Host binary not found or not executable: $BINARY"
    echo "[run-host] Build the project first (./Scripts/install-and-build.sh from project root)."
    exit 1
fi

# If SKIP_EXEC is set, only print the resolved path and exit (useful for testing)
if [[ ${SKIP_EXEC-0} -ne 0 ]]; then
    echo "[run-host] SKIP_EXEC set; not launching host."
    exit 0
fi

exec "$BINARY" "$@"
