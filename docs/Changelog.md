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

## [0.1.0] - 2026-03-01
- Project scaffold: initial commit

<!--
Rules for changelog entries:
- Use "Unreleased" for upcoming changes
- Follow semantic versioning for releases
- Keep entries concise and dated
-->
