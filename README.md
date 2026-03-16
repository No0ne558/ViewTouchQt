# ViewTouchQt

ViewTouchQt is a touchscreen-focused point-of-sale (POS) system implemented in modern C++ and Qt 6.
It provides a headless TCP server, a fullscreen Qt client with an integrated visual layout editor, and a convenience "host" binary that runs server and client together for single-machine deployments.

**Key features**
- Headless TCP server for multiple touchscreen clients
- Fullscreen touchscreen client with visual layout editor and persistent JSON layouts
- CUPS-based printing support (printer discovery & test printing)
- Compact binary protocol for low-latency messages (default port 12000)
- Designed for 1920×1080 with automatic UI scaling for other resolutions

Table of contents
- [Quick start](#quick-start)
- [Prerequisites](#prerequisites)
- [Build & install](#build--install)
- [Running the software](#running-the-software)
- [Configuration and data files](#configuration-and-data-files)
- [Remote display notes](#remote-display-notes)
- [Printing (CUPS)](#printing-cups)
- [Architecture & code tour](#architecture--code-tour)
- [Packaging and systemd service](#packaging-and-systemd-service)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License & contact](#license--contact)

Quick start
-----------

Minimum quick path (Debian/Ubuntu example): install build deps, build, run host.

```bash
# Install build dependencies (Debian/Ubuntu)
sudo apt update
sudo apt install -y build-essential cmake git pkg-config qt6-base-dev libcups2-dev libxcb-xinerama0-dev

# Out-of-source build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"

# Run host (server + main client)
./build/src/host/vt_host --port 12000
```

For a minimal developer loop you can run server and client separately (useful during development):

```bash
./build/src/server/vt_server --port 12000
./build/src/client/vt_client --server 127.0.0.1 --port 12000
```

Prerequisites
-------------

- CMake >= 3.16
- A C++17-capable toolchain (g++/clang)
- Qt 6 (this project targets Qt 6.5+; CMake allows Qt >= 6.4 but CPack metadata and runtime expectations assume Qt 6.5+)
- pkg-config and CUPS development files for printing (`libcups2-dev` on Debian/Ubuntu)
- X11/Wayland display stack for `vt_client` (client requires a GUI platform plugin)

Installable package names (Debian/Ubuntu example):

```bash
sudo apt install -y build-essential cmake git pkg-config qt6-base-dev libcups2-dev libxcb-xinerama0-dev
```

Build & install (detailed)
--------------------------

Out-of-source build (recommended):

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/viewtouch
cmake --build build -j"$(nproc)"
```

Install to the system (optional):

```bash
sudo cmake --install build --prefix /opt/viewtouch
```

Create distribution packages using CPack (from the `build` directory):

```bash
cd build
cpack -G DEB   # produces a .deb if running on a compatible distro
cpack -G RPM   # produces an .rpm where supported
```

Notes:
- The project default `CMAKE_INSTALL_PREFIX` is `/opt/viewtouch` (see [CMakeLists.txt](CMakeLists.txt#L11-L18)).
- CPack settings include a dependency on `qt6-base (>= 6.5)` for Debian packages.

Running the software
--------------------

Binaries (when built in-tree):
- Host: `build/src/host/vt_host` — convenience binary that runs server and client in one process
- Server: `build/src/server/vt_server` — headless TCP server
- Client: `build/src/client/vt_client` — fullscreen Qt client (GUI)

After installing with `cmake --install`, the binaries live under `/opt/viewtouch/bin/` (or your chosen `CMAKE_INSTALL_PREFIX`).

Common run examples:

Start host (server + client):

```bash
./build/src/host/vt_host --port 12000
```

Run server (headless):

```bash
./build/src/server/vt_server --port 12000 --bind 0.0.0.0
```

Run client (connect to a server):

```bash
./build/src/client/vt_client --server 127.0.0.1 --port 12000
```

Defaults and important constants:
- Default protocol port: 12000 (see [src/common/Protocol.h](src/common/Protocol.h#L9-L16))
- Protocol magic and message types are defined in the same header ([src/common/Protocol.h](src/common/Protocol.h)).

Configuration and data files
----------------------------

- The repository ships a sample layout: [data/default_layout.json](data/default_layout.json). When installed, CMake places this file into `${CMAKE_INSTALL_PREFIX}/dat/layout.json` (see [CMakeLists.txt](CMakeLists.txt#L33-L38)).
- The client includes an in-app editor (press F2 by default) that lets you modify the layout visually and persist changes to the configured layout path.
- Layouts are JSON files describing pages, elements, geometry, behaviour and layering. See `src/client/editor/LayoutSerializer.cpp` for the exact schema.

Remote display notes
--------------------

`vt_client` is a Qt GUI application and requires a platform plugin (XCB for X11 or a Wayland plugin). For remote or mobile X servers (for development) set `DISPLAY` and choose the platform plugin if needed:

```bash
# Example (remote X server)
DISPLAY=<remote-ip>:0 ./build/src/client/vt_client -platform xcb --server <host-ip> --port 12000

# For Wayland use the native Wayland plugin (usually automatic).
```

If you see errors about missing Qt platform plugins (`xcb`), ensure you have the Qt platform plugin packages installed (`qt6-base` / `qt6-base-dev` on distro).

Printing (CUPS)
----------------

The client integrates a `CupsPrinter` that uses `pkg-config`/CUPS for printer discovery and simple test printing. To enable printing features at build time install the CUPS development headers (`libcups2-dev` on Debian/Ubuntu).

To test printing, ensure the CUPS daemon is running and accessible from the client machine; printers will be discovered at runtime.

Architecture & code tour
------------------------

High level components:
- `src/common` — shared protocol definitions and helpers (`Protocol.h`, `Protocol.cpp`).
- `src/server` — `vt_server` (headless POS server) and session management (`PosServer.*`, `ClientSession.*`).
- `src/client` — `vt_client` (GUI client), layout engine and editor (`layout/`, `editor/`), printing (`printing/CupsPrinter.*`).
- `src/host` — `vt_host` combines server+client into a single process for convenience.

Useful entry points and references:
- [src/host/main.cpp](src/host/main.cpp) — `vt_host` entrypoint
- [src/server/main.cpp](src/server/main.cpp) — `vt_server` entrypoint
- [src/client/main.cpp](src/client/main.cpp) — `vt_client` entrypoint
- [src/common/Protocol.h](src/common/Protocol.h) — protocol constants and message header
- [src/client/layout/LayoutEngine.cpp](src/client/layout/LayoutEngine.cpp) — layout application logic
- [src/client/editor/LayoutSerializer.cpp](src/client/editor/LayoutSerializer.cpp) — layout JSON schema and serialization

Packaging and systemd service
----------------------------

Create packages with `cpack` (from the `build` dir):

```bash
cd build
cpack -G DEB   # creates a .deb (on Debian/Ubuntu)
cpack -G RPM   # creates an .rpm (on RPM-based systems)
```

Install package or use `cmake --install` to install to `/opt/viewtouch`.

Example `systemd` unit for running the headless server (`/etc/systemd/system/viewtouch-server.service`):

```ini
[Unit]
Description=ViewTouchQt POS server
After=network.target

[Service]
Type=simple
User=viewtouch
Group=viewtouch
ExecStart=/opt/viewtouch/bin/vt_server --port 12000 --bind 0.0.0.0
Restart=on-failure
LimitNOFILE=32768

[Install]
WantedBy=multi-user.target
```

Notes:
- Create a `viewtouch` user and grant it access to any required devices and the installed data directory:

```bash
sudo useradd --system --no-create-home --group nogroup viewtouch
sudo mkdir -p /opt/viewtouch/dat
sudo chown -R viewtouch: /opt/viewtouch
```

Troubleshooting
---------------

- Missing Qt or platform plugin errors: verify `qt6-base` / `qt6-base-dev` are installed and that `LD_LIBRARY_PATH` or system packaging includes the Qt plugins.
- `pkg-config` cannot find CUPS: ensure `libcups2-dev` (Debian) or `cups-devel` (Fedora) is installed.
- Build failures referencing `xcb` or X11 headers: install the appropriate X11/XCB development packages (e.g., `libxcb-xinerama0-dev`).
- Network/connectivity: server logs `PosServer` output on stdout; use `--bind 0.0.0.0` to accept external connections.

Contributing
------------

Contributions are welcome. Please:

1. Open an issue to discuss non-trivial changes before implementing.
2. Make focused branches and open pull requests against `main`.
3. Follow project coding conventions (C++17, Qt patterns). Keep changes small and well-documented.

If you'd like me to add CI configuration or packaging pipelines, open an issue and I can propose a GitHub Actions workflow.

License & contact
-----------------

This project is licensed under the GNU GPLv3 (see `LICENSE`).

Author: Ariel Brambila Pelayo — arielbrambila117@gmail.com

--
Generated README draft: review and tell me if you want any sections expanded, examples changed, or additional distro/package notes added.
