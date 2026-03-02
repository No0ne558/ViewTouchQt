// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/MainWindow.h — Fullscreen window with auto-scaling QGraphicsView.

#ifndef VT_MAIN_WINDOW_H
#define VT_MAIN_WINDOW_H

#include "PosClient.h"
#include "layout/LayoutEngine.h"

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

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void buildTestPage();

    // Design resolution — all scene items are placed in this coordinate space.
    static constexpr qreal kDesignW = 1920.0;
    static constexpr qreal kDesignH = 1080.0;

    QGraphicsView  *m_view   = nullptr;
    QGraphicsScene *m_scene  = nullptr;
    LayoutEngine   *m_engine = nullptr;
    PosClient      *m_client = nullptr;
    QString         m_lastPressedButtonId;
};

} // namespace vt

#endif // VT_MAIN_WINDOW_H
