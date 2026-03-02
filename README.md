# ViewTouchQt

Modern C++ Restaurant POS System — complete rewrite of ViewTouch.

## Overview

ViewTouchQt is a client/server POS system built with **Qt 6** and **C++17**.

| Component | Binary | Description |
|---|---|---|
| **Host** | `vt_host` | Server + main client in one process (primary entry point) |
| **Server** | `vt_server` | Headless TCP server (standalone, no GUI) |
| **Client** | `vt_client` | Fullscreen Qt Widgets app (remote terminal) |

- **Design resolution:** 1920 × 1080, auto-scales to any display size.
- **Protocol:** Raw TCP binary (port 12000 by default).
- **Platform:** Any Linux distro, X11 or Wayland. Works with XServer XSDL on Android.

## Building

```bash
# Prerequisites: CMake >= 3.16, Qt 6.5+, C++17 compiler
cmake -B build
cmake --build build
```

## Running

```bash
# Start the host (server + main client together) — recommended
./build/src/host/vt_host --port 12000

# Or run server and client separately:
./build/src/server/vt_server --port 12000
./build/src/client/vt_client --server 127.0.0.1 --port 12000

# Send a remote terminal to XServer XSDL on Android
DISPLAY=<android-ip>:0 ./build/src/client/vt_client -platform xcb --server <host-ip> --port 12000
```

See [docs/RemoteDisplay.md](docs/RemoteDisplay.md) for detailed remote display instructions.

## Project Structure

```
CMakeLists.txt              Top-level build
src/
  common/                   Shared binary protocol library
    Protocol.h / .cpp
  server/                   Headless POS server (standalone)
    PosServer.h / .cpp
    ClientSession.h / .cpp
    main.cpp
  client/                   Remote terminal client
    MainWindow.h / .cpp
    PosClient.h / .cpp
    ButtonItem.h / .cpp
    main.cpp
  host/                     Combined server + main client
    main.cpp
docs/
  Changelog.md
  RemoteDisplay.md
packaging/
  deb/                      Debian packaging (future)
  rpm/                      RPM packaging (future)
```

## License

This project is licensed under the **GNU General Public License v3** (or later).
See the `LICENSE` file for the full text.

## Contact

- **Company:** OrderPi LLC
- **Author:** Ariel Brambila Pelayo
- **Email:** arielbrambila117@gmail.com
