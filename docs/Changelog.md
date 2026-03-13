# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]
- Added `vt_host`: combined server + main client binary (primary entry point)
- Implemented `vt_server`: headless TCP server with heartbeat and client session management
- Implemented `vt_client`: fullscreen Qt Widgets terminal with auto-scaling 1920×1080 UI
- Implemented shared binary protocol library (`src/common/Protocol`)
- Added `ButtonItem`: grey button that flashes yellow on press/ack

- Editor: keep Properties dialog on top and new button behaviours (2026-03-13)
  - `PropertyDialog` is now parented to the main window and created modal with `WindowStaysOnTopHint` so it remains above the main window and cannot be lost behind it.
  - Added configurable button behaviours: `Blink` (default), `Toggle`, `None`, and `Double Tap`.
    - `Blink`: existing brief yellow flash on press/ack.
    - `Toggle`: press toggles persistent active state (visual remains active until toggled off).
    - `None`: no visual feedback on press (still emits click events).
    - `Double Tap`: requires two taps within a short timeout to trigger the action.
  - Behaviour selector added to `PropertyDialog` UI and persisted in layout JSON as `behavior`.
  - Runtime: `ButtonElement` implements behaviour branching and respects Toggle state across server ACK flashes; `LayoutSerializer` saves/restores behaviour.
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
- Added movable editor toolbar with drag handle (grip lines, open/closed hand cursors)
- Added page management system
  - `PageManagerDialog`: create, rename, delete, and navigate pages
  - `PageTabBar`: floating draggable page list panel toggled by "Page List" toolbar button
  - Page list shows element counts per page and highlights the active page
  - "Manage Pages…" link inside the floating panel opens the page manager dialog
- Added page renaming (`LayoutEngine::renamePage`) with hash re-key and signal reconnect
- Added keyboard shortcuts for the visual editor
  - Arrow keys move selected element by 10px (grid step)
  - Shift+Arrow moves selected element by 1px (fine positioning)
  - W / Shift+W increases / decreases element width by 10px
  - H / Shift+H increases / decreases element height by 10px
  - Minimum element size enforced at 40×40px
- Fixed Done button crash: deferred `setEditMode(false)` via `QTimer::singleShot(0)`
- Fixed real-time resize handle updates during drag (EditorOverlay pointer passed to ResizeHandle)
- Fixed page manager close crash: deferred `manageRequested` signal to prevent widget destruction mid-click
- Added POS system pages with Login flow
  - `PinEntryElement`: masked PIN input field with keyboard and keypad support, focus border, placeholder text
  - `KeypadButtonElement`: sends characters to PinEntry; supports BACK/CLEAR special values, flash feedback
  - `ActionButtonElement`: navigation button with action types (Login → Tables, Dine-In → Order, To-Go → Order)
  - System page framework: Login, Tables, and Order pages auto-created on first run
  - Default Login page: full keypad (0-9, Back, Clear), PIN entry, Login/Dine-In/To-Go action buttons
  - `PageWidget`: system page flag (`isSystemPage`), new factory methods for all new element types
  - `LayoutSerializer`: full save/load support for PinEntry, KeypadButton, ActionButton elements
  - `PropertyDialog`: type-specific property groups for PIN Entry, Keypad Button, Action Button
  - `LayoutEngine`: signal forwarding for keypad and action events, system/user page helpers
- Added element type change via Properties dialog
  - Type selector dropdown replaces static label — all 6 types selectable at any time
  - Type-specific property groups show/hide dynamically based on selected type
  - `PageWidget::replaceElementType()` swaps element in-place preserving common properties
  - Type-specific properties (masked, maxLength, keyValue, actionType) applied after conversion
- Fixed PropertyDialog and PageManagerDialog not movable when parent is frameless fullscreen
  - Dialogs now use `Qt::Window` flag with `nullptr` parent for truly independent top-level windows
  - Works correctly on Wayland and X11 regardless of parent window state
- Client always starts on the Login page regardless of layout load order
- Removed `+ Label` and `+ Panel` buttons from editor toolbar (use type change in Properties instead)
- Added right-click to open Properties: right-clicking an element in edit mode opens its property dialog
- Left-click drag and right-click property editing are now properly separated (no accidental drags on right-click)
- Added multi-element selection in the visual editor
  - Ctrl+click to toggle individual elements in/out of the selection
  - Drag on empty space to draw a rubber-band rectangle; all elements inside are selected
  - Move all selected elements together by dragging or with arrow keys
  - Delete key removes all selected elements at once
  - Resize handles shown only for single-element selection
  - Ctrl+click on empty space preserves existing selection while starting rubber-band
- Added configurable font styles for POS system elements
  - `UiElement::fontFamily` property (default: "Sans") with curated POS-friendly font list
  - `UiElement::fontBold` toggle (default: on)
  - `buildFont()` helper replaces hardcoded QFont construction in all 6 element types
  - PropertyDialog: font family combo (17 options) and bold checkbox in Text group
  - LayoutSerializer: full save/load for fontFamily and fontBold
  - Available fonts: Sans, Noto Sans, Open Sans, Montserrat, Liberation Sans, Cantarell,
    DejaVu Sans, Droid Sans, Comfortaa, Carlito, Caladea, Serif, Liberation Serif,
    Nimbus Roman, Monospace, Liberation Mono, Source Code Pro
- Added `Logout` action type for ActionButton — navigates back to the Login page
- Added `Navigation` action type for ActionButton — navigate to any arbitrary page by name
  - Target page selectable via combo box in PropertyDialog when Navigation action is selected
  - Signal chain updated to carry target page name (ActionButton → PageWidget → LayoutEngine → MainWindow)
  - Full serialization support for targetPage in LayoutSerializer
- Added page inheritance system
  - `UiElement::inheritable` flag (default off) — marks elements as available for inheritance
  - `PageWidget::inheritFrom` property — specifies which page to inherit elements from
  - `LayoutEngine` attaches inheritable elements from parent page to scene on page switch
  - Inheritable checkbox in PropertyDialog; inherit-from combo in PageManagerDialog
  - Full serialization support for inheritable and inheritFrom in LayoutSerializer
- Redesigned PageManagerDialog with split layout
  - Left panel: sortable page list with element counts, active page indicator, system page badges
  - Right panel: page properties (name, system page flag, inherit-from combo, apply button)
  - Rename propagation: updates inheritFrom references when a page is renamed
  - Delete cleanup: clears inheritFrom references when a page is deleted
- Added layout synchronization over TCP
  - Server pushes full layout JSON to every connected client on edit-mode save
  - New clients receive the current layout immediately on connection
  - Protocol upgraded: payload length expanded from uint16 to uint32 (header now 10 bytes)
  - New `MsgType::LayoutSync` (0x0020) carries compact JSON layout payload
  - `PosServer::setCurrentLayout()` caches and broadcasts layout to all sessions
  - `PosClient::layoutSyncReceived` signal emitted on incoming layout sync
  - `MainWindow::layoutChanged` signal emitted on initial load and edit-mode save
  - `MainWindow::applyLayoutFromNetwork()` deserializes and replaces local layout
  - Host wiring: layout changes flow from MainWindow → PosServer → all remote clients
  - Client wiring: layout sync received from server is applied automatically
  - Each client remains an independent terminal (own POS state, own interactions)
- Added display management system with CUPS printer integration
  - New "Displays" system page: lists configured terminals with name, IP, printer, active status
  - Selection-based CRUD: Add (green), Edit (blue), Remove (red), Toggle active (yellow) buttons
  - New "DisplayEdit" system page: form for Display Name, Display IP Address, Printer, Test Printer, Done
  - Built-in full QWERTY keyboard on DisplayEdit (5 rows: numbers/punct, QWERTYUIOP, ASDFGHJKL, ZXCVBNM, Back/Space/Clear)
  - CUPS printer discovery via `CupsPrinter::availablePrinters()` (uses `cupsGetDests()`)
  - Printer field cycles through discovered CUPS printers on tap
  - ESC/POS thermal test print: 80mm / 42-char receipt with header, dashes, body, and partial auto-cut (`GS V B 3`)
  - Raw print job submission via `cupsCreateJob()` / `cupsWriteRequestData()` / `cupsFinishDocument()`
  - `DisplayConfig` data model: UUID, name, IP, printer, active flag; JSON serialization
  - `DisplayManager`: CRUD + JSON persistence to `~/.config/ViewTouchQt/displays.json`
  - 7 new `ActionType` values: ShowDisplays, AddDisplay, EditDisplay, RemoveDisplay, ToggleDisplay, TestPrinter, DisplayDone
  - PropertyDialog updated with all new action types; fixed combo index mapping via `findData()`
  - Refactored `wirePageKeypad()` to support multiple PinEntry fields per page using Qt focus system

  ### 2026-03-11 — Button-only cleanup

  - Removed all non-Button UI element types and their implementations: `Label`, `Panel`, `PinEntry`, `KeypadButton`, `ActionButton`, `InfoLabel`.
    - Deleted files (client layout):
      - `src/client/layout/LabelElement.h`, `src/client/layout/LabelElement.cpp`
      - `src/client/layout/PanelElement.h`, `src/client/layout/PanelElement.cpp`
      - `src/client/layout/PinEntryElement.h`, `src/client/layout/PinEntryElement.cpp`
      - `src/client/layout/KeypadButtonElement.h`, `src/client/layout/KeypadButtonElement.cpp`
      - `src/client/layout/ActionButtonElement.h`, `src/client/layout/ActionButtonElement.cpp`
      - `src/client/layout/InfoLabelElement.h`, `src/client/layout/InfoLabelElement.cpp`

  - Simplified editor and runtime behavior to Button-only:
    - `PropertyDialog` now exposes only Button properties and no PIN/keypad/action UI.
    - `LayoutSerializer` and editor UI only serialize/deserialize `Button` elements.
    - `MainWindow` enforces a single minimal `Login` system page at startup.

  - Build system updates:
    - Removed deleted sources from `src/client/CMakeLists.txt` and `src/host/CMakeLists.txt` so removed files are no longer compiled.

  - Rationale: reduce feature scope to a minimal, stable runtime while preserving the layout engine and Button element for incremental re-introduction later.

  - Verification: performed a full local build — `vt_client`, `vt_host`, and `vt_server` all built successfully after these changes.

  - Editor cleanup (2026-03-13)

    - Removed leftover action-type UI placeholder from `PropertyDialog`:
      - Deleted the `m_actionTypeCombo` member and simplified `PropertyDialog::actionTypeValue()` to return 0.
      - This removes the last UI affordance for ActionButton configuration in the simplified Button-only editor (commit ff99ce8).

## [0.1.0] - 2026-03-01
- Project scaffold: initial commit

<!--
Rules for changelog entries:
- Use "Unreleased" for upcoming changes
- Follow semantic versioning for releases
- Keep entries concise and dated
-->
