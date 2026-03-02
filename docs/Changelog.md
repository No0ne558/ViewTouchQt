# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]
- Added `vt_host`: combined server + main client binary (primary entry point)
- Implemented `vt_server`: headless TCP server with heartbeat and client session management
- Implemented `vt_client`: fullscreen Qt Widgets terminal with auto-scaling 1920×1080 UI
- Implemented shared binary protocol library (`src/common/Protocol`)
- Added `ButtonItem`: grey button that flashes yellow on press/ack
- Added `PosClient` with automatic reconnection (3 s retry)
- Added `ClientSession` with heartbeat (5 s ping, 15 s timeout)
- Added CMake build system with Qt 6.5+ / C++17
- Added CPack support for deb/rpm packaging
- Added `docs/RemoteDisplay.md` with XServer XSDL guide
- Changed default port from 9100 to 12000 (avoids JetDirect printer conflicts)
- Added `-platform xcb` to remote display docs for Wayland host sessions
- Verified remote `vt_client` mirrors host UI as independent client session
- Fixed fullscreen on XServer XSDL: frameless window + manual screen geometry fallback
- Added `showEvent` with deferred `fitInView` for reliable scaling on remote X11 displays
- Added programmatic layout engine (`src/client/layout/`)
  - `UiElement`: base class with position, size, colour, label, font size, corner radius
  - `ButtonElement`: touchable button with flash feedback and configurable active colour
  - `LabelElement`: static text with alignment and optional background
  - `PanelElement`: background container/divider with optional border
  - `PageWidget`: named page that owns elements with add/remove/attach/detach
  - `LayoutEngine`: manages pages, page switching, forwards button clicks
- Replaced hardcoded `ButtonItem` with layout engine in `MainWindow`
- Test page demonstrates header panel, title label, two buttons, footer, status label
- Fixed per-button flash: only the pressed button flashes on server ack
- Fixed `setBgColor` sync: ButtonElement keeps `m_currentColor` in sync with background colour
- Added built-in visual drag-and-drop editor (`src/client/editor/`)
  - `EditorOverlay`: edit mode toggle (F2), element selection, drag to move, snap-to-grid (10px)
  - `ResizeHandle`: 8-point resize handles (corners + edges) with per-axis cursors
  - `PropertyDialog`: modal dialog to edit element ID, label, position, size, colours, corner radius
  - `LayoutSerializer`: save/load full layout to/from JSON (`~/.config/ViewTouchQt/layout.json`)
  - Toolbar overlay: add Button / Label / Panel, Delete, Properties, Done buttons
  - Double-click element to open property editor; Delete key removes selected element
  - Layout auto-saves when exiting edit mode; auto-loads on startup if file exists

## [0.1.0] - 2026-03-01
- Project scaffold: initial commit

<!--
Rules for changelog entries:
- Use "Unreleased" for upcoming changes
- Follow semantic versioning for releases
- Keep entries concise and dated
-->
