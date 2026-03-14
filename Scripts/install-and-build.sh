#!/usr/bin/env bash
set -euo pipefail

# install-and-build.sh
# Detects the host Linux distribution's package manager, attempts a best-effort
# install of build dependencies, then configures and builds the project.
# Usage: ./install-and-build.sh [--no-install]

SKIP_INSTALL=0
if [[ ${1-} == "--no-install" ]]; then
    SKIP_INSTALL=1
fi

echo "[installer] Working directory: $(pwd)"

detect_pm() {
    if command -v apt-get >/dev/null 2>&1; then
        echo apt
    elif command -v dnf >/dev/null 2>&1; then
        echo dnf
    elif command -v yum >/dev/null 2>&1; then
        echo yum
    elif command -v pacman >/dev/null 2>&1; then
        echo pacman
    elif command -v zypper >/dev/null 2>&1; then
        echo zypper
    elif command -v apk >/dev/null 2>&1; then
        echo apk
    else
        echo unknown
    fi
}

PM=$(detect_pm)
if [[ $PM == unknown ]]; then
    echo "[installer] Warning: could not detect package manager. You may need to install dependencies manually."
fi

install_packages() {
    echo "[installer] Attempting to install common build dependencies for detected package manager: $PM"
    case "$PM" in
        apt)
            sudo apt-get update || true
            sudo apt-get install -y build-essential cmake git pkg-config \ 
                libxcb-xinerama0-dev libxcb1-dev qt6-base-dev qtbase5-dev || true
            ;;
        dnf)
            sudo dnf install -y @development-tools cmake git pkgconfig \ 
                xcb-util-wm-devel libxcb-devel qt6-qtbase-devel || true
            ;;
        yum)
            sudo yum groupinstall -y "Development Tools" || true
            sudo yum install -y cmake git pkgconfig libxcb-devel qt5-qtbase-devel || true
            ;;
        pacman)
            sudo pacman -Sy --noconfirm base-devel cmake git pkgconf qt6-base || true
            ;;
        zypper)
            sudo zypper install -t pattern devel_C_C++ || true
            sudo zypper install -y cmake git pkg-config libxcb-devel libqt6-qtbase-devel || true
            ;;
        apk)
            sudo apk add build-base cmake git pkgconfig qt6-qtbase-dev libxcb-dev || true
            ;;
        *)
            echo "[installer] Unknown package manager. Skipping automatic installs."
            ;;
    esac
    echo "[installer] Done attempted installs (errors ignored). If dependencies failed, please install them manually."
}

if [[ $SKIP_INSTALL -eq 0 ]]; then
    install_packages
else
    echo "[installer] Skipping package installation (--no-install provided)."
fi

# Configure & build
BUILD_DIR=build
mkdir -p "$BUILD_DIR"

echo "[builder] Running CMake configure"
cmake -S . -B "$BUILD_DIR"

echo "[builder] Building project (parallel jobs: "+$(nproc)+")"
cmake --build "$BUILD_DIR" -- -j"$(nproc)"

echo "[builder] Build finished. Binaries located in $BUILD_DIR/src/..."
