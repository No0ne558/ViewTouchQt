// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/MainWindow.h — Fullscreen window with auto-scaling QGraphicsView.

#ifndef VT_MAIN_WINDOW_H
#define VT_MAIN_WINDOW_H

#include "ButtonItem.h"
#include "PosClient.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMainWindow>

namespace vt {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    /// @param client  The network client (not owned — must outlive this window).
    explicit MainWindow(PosClient *client, QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void setupScene();

    // Design resolution — all scene items are placed in this coordinate space.
    static constexpr qreal kDesignW = 1920.0;
    static constexpr qreal kDesignH = 1080.0;

    QGraphicsView  *m_view  = nullptr;
    QGraphicsScene *m_scene = nullptr;
    ButtonItem     *m_button = nullptr;
    PosClient      *m_client = nullptr;
};

} // namespace vt

#endif // VT_MAIN_WINDOW_H
