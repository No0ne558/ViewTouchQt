// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/MainWindow.h — Fullscreen window with auto-scaling QGraphicsView.

#ifndef VT_MAIN_WINDOW_H
#define VT_MAIN_WINDOW_H

#include "PosClient.h"
#include "layout/LayoutEngine.h"
#include "editor/EditorOverlay.h"
#include "displays/DisplayManager.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMainWindow>

namespace vt {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    /// @param client  The network client (not owned — must outlive this window).
    explicit MainWindow(PosClient *client, QWidget *parent = nullptr);

    /// Access the layout engine for runtime page/element manipulation.
    LayoutEngine *layoutEngine() { return m_engine; }

    /// Access the editor overlay.
    EditorOverlay *editorOverlay() { return m_editor; }

    /// Apply a layout received from the server (replaces current layout).
    void applyLayoutFromNetwork(const QByteArray &layoutJson);

signals:
    /// Emitted whenever the authoritative layout changes (edit-mode save, initial load).
    void layoutChanged(const QByteArray &layoutJson);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void buildTestPage();
    void buildDefaultLoginPage();
    void buildDisplaysPage();
    void buildDisplayEditPage();
    void buildDisplayEditKeyboard(PageWidget *pg);
    void ensureSystemPages();
    void wirePageKeypad(const QString &pageName);
    void handleAction(const QString &pageName, ActionType action, const QString &targetPage);
    void toggleEditMode();
    void openPropertyDialog(UiElement *elem);
    void openPageManager();

    // ── Display management helpers ──────────────────────────────────────
    void refreshDisplayList();
    void handleAddDisplay();
    void handleEditDisplay();
    void handleRemoveDisplay();
    void handleToggleDisplay();
    void handleTestPrinter();
    void handleDisplayDone();

    /// Try to load layout from the default file path; returns true on success.
    bool loadLayoutIfExists();

    /// Default layout file path (next to the executable or in config dir).
    QString defaultLayoutPath() const;

    // Design resolution — all scene items are placed in this coordinate space.
    static constexpr qreal kDesignW = 1920.0;
    static constexpr qreal kDesignH = 1080.0;

    QGraphicsView   *m_view   = nullptr;
    QGraphicsScene  *m_scene  = nullptr;
    LayoutEngine    *m_engine = nullptr;
    EditorOverlay   *m_editor = nullptr;
    PosClient       *m_client = nullptr;
    DisplayManager  *m_displayMgr = nullptr;
    QString          m_lastPressedButtonId;
    int              m_selectedDisplayIdx = -1;   // index in DisplayManager
    QString          m_editingDisplayUuid;        // UUID of display being edited
};

} // namespace vt

#endif // VT_MAIN_WINDOW_H
