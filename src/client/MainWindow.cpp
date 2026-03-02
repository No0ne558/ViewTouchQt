// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/MainWindow.cpp

#include "MainWindow.h"

#include <QResizeEvent>

namespace vt {

MainWindow::MainWindow(PosClient *client, QWidget *parent)
    : QMainWindow(parent)
    , m_client(client)
{
    // ── Graphics view / scene ───────────────────────────────────────────
    m_scene = new QGraphicsScene(0, 0, kDesignW, kDesignH, this);
    m_scene->setBackgroundBrush(QColor(30, 30, 30));  // dark background

    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setFrameShape(QFrame::NoFrame);

    // Accept touch events on the viewport widget.
    m_view->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);

    setCentralWidget(m_view);

    setupScene();

    // ── Fullscreen ──────────────────────────────────────────────────────
    setWindowState(Qt::WindowFullScreen);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    // Scale the 1920×1080 scene to fill the window, preserving aspect ratio.
    m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::setupScene()
{
    // Centre a 400×120 button in the 1920×1080 design space.
    constexpr qreal bw = 400;
    constexpr qreal bh = 120;
    const qreal bx = (kDesignW - bw) / 2.0;
    const qreal by = (kDesignH - bh) / 2.0;

    m_button = new ButtonItem(bx, by, bw, bh);
    m_scene->addItem(m_button);

    // Button press → send to server.
    connect(m_button, &ButtonItem::pressed, this, [this]() {
        m_client->send(MsgType::ButtonPress);
    });

    // Server ack → flash the button (second visual confirmation).
    connect(m_client, &PosClient::buttonAckReceived, m_button, &ButtonItem::flashAck);
}

} // namespace vt
