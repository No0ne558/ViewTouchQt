// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/MainWindow.cpp

#include "MainWindow.h"
#include "editor/PropertyDialog.h"
#include "editor/LayoutSerializer.h"
#include "editor/PageManagerDialog.h"

#include <QCoreApplication>
#include <QDir>
#include <QKeyEvent>
#include <QMessageBox>
#include <QResizeEvent>
#include <QScreen>
#include <QShowEvent>
#include <QStandardPaths>
#include <QTimer>

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

    // ── Layout engine ───────────────────────────────────────────────────
    m_engine = new LayoutEngine(m_scene, this);

    // Forward any button click to the server and track which button was pressed.
    connect(m_engine, &LayoutEngine::buttonClicked, this,
            [this](const QString & /*pageName*/, const QString &elementId) {
                m_lastPressedButtonId = elementId;
                m_client->send(MsgType::ButtonPress);
            });

    // Server ack → flash only the button that was pressed.
    connect(m_client, &PosClient::buttonAckReceived, this, [this]() {
        if (m_lastPressedButtonId.isEmpty())
            return;
        if (auto *pg = m_engine->activePage()) {
            auto *elem = pg->element(m_lastPressedButtonId);
            if (elem && elem->elementType() == ElementType::Button) {
                static_cast<ButtonElement *>(elem)->flashAck();
            }
        }
        m_lastPressedButtonId.clear();
    });

    // ── Editor overlay ──────────────────────────────────────────────────
    m_editor = new EditorOverlay(m_engine, m_scene, this);
    connect(m_editor, &EditorOverlay::editPropertiesRequested, this,
            &MainWindow::openPropertyDialog);
    connect(m_editor, &EditorOverlay::pageManagerRequested, this,
            &MainWindow::openPageManager);
    connect(m_editor, &EditorOverlay::editModeChanged, this, [this](bool on) {
        if (!on) {
            // Auto-save layout when exiting edit mode
            QString path = defaultLayoutPath();
            LayoutSerializer::saveToFile(m_engine, path);
        }
    });

    // ── Load layout or build default ────────────────────────────────────
    if (!loadLayoutIfExists())
        buildTestPage();

    // ── Fullscreen ──────────────────────────────────────────────────────
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setWindowState(Qt::WindowFullScreen);

    if (QScreen *scr = screen()) {
        setGeometry(scr->geometry());
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);

    auto applyFullscreen = [this]() {
        if (QScreen *scr = screen()) {
            setGeometry(scr->geometry());
        }
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    };

    QTimer::singleShot(100, this, applyFullscreen);
    QTimer::singleShot(500, this, applyFullscreen);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_F2:
        toggleEditMode();
        return;
    case Qt::Key_Escape:
        if (m_editor->isEditMode()) {
            m_editor->setEditMode(false);
            return;
        }
        break;
    case Qt::Key_Delete:
        if (m_editor->isEditMode()) {
            m_editor->deleteSelected();
            return;
        }
        break;
    }
    QMainWindow::keyPressEvent(event);
}

// ── Edit mode ───────────────────────────────────────────────────────────────

void MainWindow::toggleEditMode()
{
    m_editor->setEditMode(!m_editor->isEditMode());
}

void MainWindow::openPropertyDialog(UiElement *elem)
{
    PropertyDialog dlg(elem, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_editor->deselectAll();
        m_editor->selectElement(elem);  // refresh handles after resize
    }
}

void MainWindow::openPageManager()
{
    PageManagerDialog dlg(m_engine, this);
    dlg.exec();

    // Refresh the page tab bar to reflect any changes
    m_editor->pageTabBar()->refresh();
    m_editor->deselectAll();
}

// ── Layout persistence ──────────────────────────────────────────────────────

QString MainWindow::defaultLayoutPath() const
{
    // Store layout in config directory: ~/.config/ViewTouchQt/layout.json
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/layout.json");
}

bool MainWindow::loadLayoutIfExists()
{
    QString path = defaultLayoutPath();
    if (!QFile::exists(path))
        return false;

    return LayoutSerializer::loadFromFile(m_engine, path);
}

// ── Test page ───────────────────────────────────────────────────────────────

void MainWindow::buildTestPage()
{
    auto *pg = m_engine->createPage(QStringLiteral("test"));

    // Header panel
    auto *headerPanel = pg->addPanel(QStringLiteral("header_bg"), 0, 0, 1920, 80);
    headerPanel->setBgColor(QColor(45, 45, 45));
    headerPanel->setCornerRadius(0);

    // Title label
    auto *title = pg->addLabel(QStringLiteral("title"), 20, 10, 600, 60,
                               QStringLiteral("ViewTouchQt POS"));
    title->setFontSize(36);
    title->setTextColor(Qt::white);
    title->setAlignment(Qt::AlignLeft);

    // Centre button (same as the original demo button)
    auto *btn = pg->addButton(QStringLiteral("btn_press"),
                              760, 480, 400, 120,
                              QStringLiteral("PRESS"));
    btn->setBgColor(QColor(160, 160, 160));
    btn->setFontSize(32);

    // A second button as a demonstration of the layout engine
    auto *btn2 = pg->addButton(QStringLiteral("btn_demo"),
                               760, 640, 400, 80,
                               QStringLiteral("DEMO BUTTON"));
    btn2->setBgColor(QColor(80, 140, 200));
    btn2->setTextColor(Qt::white);
    btn2->setFontSize(24);
    btn2->setActiveColor(QColor(120, 200, 255));

    // Footer panel
    auto *footer = pg->addPanel(QStringLiteral("footer_bg"), 0, 1000, 1920, 80);
    footer->setBgColor(QColor(45, 45, 45));
    footer->setCornerRadius(0);

    // Status label in footer
    auto *status = pg->addLabel(QStringLiteral("status"), 20, 1010, 800, 60,
                                QStringLiteral("Status: Ready"));
    status->setFontSize(22);
    status->setTextColor(QColor(150, 255, 150));
    status->setAlignment(Qt::AlignLeft);

    // Edit hint
    auto *hint = pg->addLabel(QStringLiteral("edit_hint"), 1400, 1020, 500, 40,
                              QStringLiteral("Press F2 to edit layout"));
    hint->setFontSize(18);
    hint->setTextColor(QColor(120, 120, 120));
    hint->setAlignment(Qt::AlignRight);

    m_engine->showPage(QStringLiteral("test"));
}

} // namespace vt
