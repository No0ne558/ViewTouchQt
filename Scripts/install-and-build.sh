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

    # Candidate package names for each role. We'll probe availability and only
    # install packages that exist in the system repositories.
    BUILD_PKGS_COMMON=(cmake git)

    case "$PM" in
        apt)
            sudo apt-get update || true
            PKG_QUERY_CMD() { apt-cache show "$1" >/dev/null 2>&1; }
            INSTALL_CMD() { sudo apt-get install -y "$@"; }
            # Candidate packages
            BUILD_TOOLS=(build-essential)
            PKGCONFIG=(pkg-config)
            XCB_CANDIDATES=(libxcb-xinerama0-dev libxcb1-dev libxcb-devel)
            QT_CANDIDATES=(qt6-base-dev qtbase5-dev libqt6-dev libqt5-dev)
            ;;
        dnf)
            PKG_QUERY_CMD() { dnf info "$1" >/dev/null 2>&1; }
            INSTALL_CMD() { sudo dnf install -y "$@"; }
            BUILD_TOOLS=('@development-tools')
            PKGCONFIG=(pkgconfig)
            XCB_CANDIDATES=(xcb-util-wm-devel libxcb-devel)
            QT_CANDIDATES=(qt6-qtbase-devel qt5-qtbase-devel)
            ;;
        yum)
            PKG_QUERY_CMD() { yum info "$1" >/dev/null 2>&1; }
            INSTALL_CMD() { sudo yum install -y "$@"; }
            BUILD_TOOLS=('Development Tools')
            PKGCONFIG=(pkgconfig)
            XCB_CANDIDATES=(libxcb-devel)
            QT_CANDIDATES=(qt5-qtbase-devel qt6-qtbase-devel)
            ;;
        pacman)
            PKG_QUERY_CMD() { pacman -Si "$1" >/dev/null 2>&1; }
            INSTALL_CMD() { sudo pacman -Sy --noconfirm "$@"; }
            BUILD_TOOLS=(base-devel)
            PKGCONFIG=(pkgconf)
            XCB_CANDIDATES=(libxcb)
            QT_CANDIDATES=(qt6-base qt5-base)
            ;;
        zypper)
            PKG_QUERY_CMD() { zypper se -s "$1" >/dev/null 2>&1; }
            INSTALL_CMD() { sudo zypper install -y "$@"; }
            BUILD_TOOLS=(devel_C_C++)
            PKGCONFIG=(pkg-config)
            XCB_CANDIDATES=(libxcb-devel)
            QT_CANDIDATES=(libqt6-qtbase-devel libqt5-qtbase-devel)
            ;;
        apk)
            PKG_QUERY_CMD() { apk search "$1" >/dev/null 2>&1; }
            INSTALL_CMD() { sudo apk add "$@"; }
            BUILD_TOOLS=(build-base)
            PKGCONFIG=(pkgconfig)
            XCB_CANDIDATES=(libxcb-dev)
            QT_CANDIDATES=(qt6-qtbase-dev qt5-qtbase-dev)
            ;;
        *)
            echo "[installer] Unknown package manager. Skipping automatic installs."
            return
            ;;
    esac

    # Helper to check candidate list and pick available ones
    pick_available() {
        local -n candidates=$1
        shift
        local -n out=$1
        out=()
        for pkg in "${candidates[@]}"; do
            if PKG_QUERY_CMD "$pkg"; then
                out+=("$pkg")
            fi
        done
    }

    # Build list to install
    TO_INSTALL=()

    # Add build tools (group/package)
    for t in "${BUILD_TOOLS[@]:-}"; do
        # Some package managers use group install (dnf) — try the install command
        # directly for groups if query isn't available
        if PKG_QUERY_CMD "$t"; then
            TO_INSTALL+=("$t")
        else
            # If query failed, still try to include it (install may succeed)
            TO_INSTALL+=("$t")
        fi
    done

    # Common small packages
    for p in "${BUILD_PKGS_COMMON[@]}"; do
        if PKG_QUERY_CMD "$p"; then
            TO_INSTALL+=("$p")
        fi
    done

    # pkg-config
    for p in "${PKGCONFIG[@]:-}"; do
        if PKG_QUERY_CMD "$p"; then TO_INSTALL+=("$p"); break; fi
    done

    # XCB dev
    pick_available XCB_CANDIDATES XCB_AVAIL
    for p in "${XCB_AVAIL[@]:-}"; do TO_INSTALL+=("$p"); done

    # Qt dev
    pick_available QT_CANDIDATES QT_AVAIL
    if [ ${#QT_AVAIL[@]} -gt 0 ]; then
        # Prefer the first available candidate
        TO_INSTALL+=("${QT_AVAIL[0]}")
    fi

    if [ ${#TO_INSTALL[@]} -eq 0 ]; then
        echo "[installer] No packages detected for installation. Please install dependencies manually."
        return
    fi

    echo "[installer] Installing packages: ${TO_INSTALL[*]}"
    INSTALL_CMD "${TO_INSTALL[@]}" || true
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
