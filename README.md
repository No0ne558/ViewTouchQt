# ViewTouchQt

ViewTouchQt is a lightweight POS (point-of-sale) system implemented in modern C++ with Qt 6.

This repository contains a combined host application, a headless server, and a fullscreen client intended for touchscreen terminals.

Key binaries
- `vt_host` — host process (recommended): runs server + main client together
- `vt_server` — standalone headless TCP server
- `vt_client` — fullscreen Qt client (remote terminal)

Highlights
- Design resolution: 1920×1080 with automatic scaling for arbitrary displays
- Simple binary TCP protocol (default port 12000)
- Editor mode: visual layout editor with movable elements and persistent layout JSON

Quick start (build)

Prerequisites: CMake >= 3.16, Qt 6.5+, a C++17 compiler

```bash
cmake -B build
cmake --build build -j$(nproc)
```

Run

Start host (server + client):

```bash
./build/src/host/vt_host --port 12000
```

Run server and client separately (useful for development):

```bash
./build/src/server/vt_server --port 12000
./build/src/client/vt_client --server 127.0.0.1 --port 12000
```

Remote display (XServer/X11)

To display the client on a remote X server (e.g. XServer XSDL on Android), set `DISPLAY` and run the client with the XCB platform plugin:

```bash
DISPLAY=<remote-ip>:0 ./build/src/client/vt_client -platform xcb --server <host-ip> --port 12000
```

Editor & runtime features

- Visual layout engine with `UiElement` and `ButtonElement` types
- In-app editor (F2) with selection, move/resize handles, and `PropertyDialog` for per-element settings
- PropertyDialog improvements:
  - Dialog is modal and parented to the main window and marked `WindowStaysOnTopHint` so it remains visible above the fullscreen UI
  - New `Behaviour` setting for buttons: `Blink` (default), `Toggle`, `None`, `Double Tap`
    - `Blink`: short yellow flash on press/ack
    - `Toggle`: persistent on/off visual state, toggled on press
    - `None`: no visual feedback (events still emitted)
    - `Double Tap`: requires two taps within a short timeout to trigger
  - New `Layer` setting (-10..10) controls stacking order; higher values draw above lower ones. Layer is persisted in layout JSON and mapped to the QGraphics `zValue`.

Persistence

- Layouts are saved/loaded as JSON in the application config directory (see `MainWindow::defaultLayoutPath()` — typically `~/.config/ViewTouchQt/layout.json`).
- `LayoutSerializer` handles serialization of element geometry, colours, behaviour, and layer.

Project structure

- `src/common` — shared protocol and helpers
- `src/server` — headless TCP server
- `src/client` — fullscreen Qt client and editor
- `src/host` — host binary (server + client)
- `docs` — changelog and platform-specific notes

Contributing

Contributions welcome. Please open issues for bugs or feature requests, and send PRs against `main` with focused changes.

License

This project is licensed under GPLv3 (see `LICENSE` file).

Contact

- Author: Ariel Brambila Pelayo — arielbrambila117@gmail.com
