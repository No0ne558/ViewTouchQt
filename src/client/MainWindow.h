// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/MainWindow.h — Fullscreen window with auto-scaling QGraphicsView.

#ifndef VT_MAIN_WINDOW_H
#define VT_MAIN_WINDOW_H

#include "PosClient.h"
#include "layout/LayoutEngine.h"
#include "editor/EditorOverlay.h"

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

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void buildTestPage();
    void toggleEditMode();
    void openPropertyDialog(UiElement *elem);
    void openPageManager();

    /// Try to load layout from the default file path; returns true on success.
    bool loadLayoutIfExists();

    /// Default layout file path (next to the executable or in config dir).
    QString defaultLayoutPath() const;

    // Design resolution — all scene items are placed in this coordinate space.
    static constexpr qreal kDesignW = 1920.0;
    static constexpr qreal kDesignH = 1080.0;

    QGraphicsView  *m_view   = nullptr;
    QGraphicsScene *m_scene  = nullptr;
    LayoutEngine   *m_engine = nullptr;
    EditorOverlay  *m_editor = nullptr;
    PosClient      *m_client = nullptr;
    QString         m_lastPressedButtonId;
};

} // namespace vt

#endif // VT_MAIN_WINDOW_H
