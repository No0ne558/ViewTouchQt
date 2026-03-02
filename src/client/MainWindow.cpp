// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/MainWindow.cpp

#include "MainWindow.h"
#include "editor/PropertyDialog.h"
#include "editor/LayoutSerializer.h"
#include "editor/PageManagerDialog.h"
#include "layout/PinEntryElement.h"
#include "layout/KeypadButtonElement.h"
#include "layout/ActionButtonElement.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
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

    // Keypad button presses → route to PinEntry on the same page
    connect(m_engine, &LayoutEngine::keypadPressed, this,
            [this](const QString &pageName, const QString &value) {
                wirePageKeypad(pageName);
                Q_UNUSED(value); // actual wiring is page-local, see wirePageKeypad
            });

    // Action button presses → navigation
    connect(m_engine, &LayoutEngine::actionTriggered, this,
            &MainWindow::handleAction);

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

            // Notify (host wiring broadcasts this to clients)
            QJsonObject root = LayoutSerializer::serialize(m_engine);
            QJsonDocument doc(root);
            emit layoutChanged(doc.toJson(QJsonDocument::Compact));
        }
    });

    // ── Load layout or build default ────────────────────────────────────
    if (!loadLayoutIfExists())
        buildTestPage();

    // Ensure system pages always exist (Login, Tables, Order).
    ensureSystemPages();

    // ── Fullscreen ──────────────────────────────────────────────────────
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setWindowState(Qt::WindowFullScreen);

    // Use actual screen geometry when available, avoid hardcoding 1920×1080
    // which would over-size the window on smaller remote displays.
    if (QScreen *scr = screen()) {
        QRect geo = scr->geometry();
        if (geo.width() > 0 && geo.height() > 0)
            setGeometry(geo);
    }

    // Defer the initial layout broadcast so that main.cpp has time to
    // wire layoutChanged → PosServer::setCurrentLayout before it fires.
    QTimer::singleShot(0, this, [this]() {
        QJsonObject root = LayoutSerializer::serialize(m_engine);
        QJsonDocument doc(root);
        emit layoutChanged(doc.toJson(QJsonDocument::Compact));
    });
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
        showFullScreen();
        if (QScreen *scr = screen()) {
            QRect geo = scr->availableGeometry();
            if (geo.width() > 0 && geo.height() > 0)
                setGeometry(geo);
        }
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    };

    QTimer::singleShot(100, this, applyFullscreen);
    QTimer::singleShot(500, this, applyFullscreen);
    QTimer::singleShot(1500, this, applyFullscreen);
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
    PropertyDialog dlg(elem, nullptr);  // nullptr parent → independent top-level window
    dlg.setPageNames(m_engine->pageNames());
    if (dlg.exec() == QDialog::Accepted) {
        UiElement *target = elem;

        // If the user changed the element type, replace it on the page
        if (dlg.typeChanged()) {
            auto *page = m_engine->activePage();
            if (page) {
                auto *replacement = page->replaceElementType(
                    elem->elementId(), dlg.newType());
                if (replacement) {
                    target = replacement;

                    // Apply type-specific properties from the dialog
                    switch (dlg.newType()) {
                    case ElementType::PinEntry: {
                        auto *pin = static_cast<PinEntryElement *>(replacement);
                        pin->setMasked(dlg.pinMasked());
                        pin->setMaxLength(dlg.pinMaxLength());
                        break;
                    }
                    case ElementType::KeypadButton: {
                        auto *kpd = static_cast<KeypadButtonElement *>(replacement);
                        kpd->setKeyValue(dlg.keypadValue());
                        break;
                    }
                    case ElementType::ActionButton: {
                        auto *act = static_cast<ActionButtonElement *>(replacement);
                        act->setActionType(static_cast<ActionType>(dlg.actionTypeValue()));
                        if (act->actionType() == ActionType::Navigation)
                            act->setTargetPage(dlg.targetPage());
                        break;
                    }
                    default:
                        break;
                    }
                }
            }
        }

        m_editor->deselectAll();
        m_editor->selectElement(target);  // refresh handles after resize
    }
}

void MainWindow::openPageManager()
{
    PageManagerDialog dlg(m_engine, nullptr);  // nullptr parent → independent top-level window
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

// ── System pages ────────────────────────────────────────────────────────────

void MainWindow::ensureSystemPages()
{
    // Create system pages if they don't already exist (e.g., after first run
    // or if layout.json doesn't include them).
    if (!m_engine->page(QStringLiteral("Login"))) {
        buildDefaultLoginPage();
    } else {
        // Mark existing Login page as system
        m_engine->page(QStringLiteral("Login"))->setSystemPage(true);
    }

    if (!m_engine->page(QStringLiteral("Tables"))) {
        auto *pg = m_engine->createPage(QStringLiteral("Tables"));
        pg->setSystemPage(true);
        // Minimal placeholder content
        auto *title = pg->addLabel(QStringLiteral("tables_title"), 20, 10, 600, 60,
                                    QStringLiteral("Tables"));
        title->setFontSize(36);
        title->setTextColor(Qt::white);
        title->setAlignment(Qt::AlignLeft);

        auto *hint = pg->addLabel(QStringLiteral("tables_hint"), 560, 500, 800, 60,
                                   QStringLiteral("Configure this page in the editor (F2)"));
        hint->setFontSize(20);
        hint->setTextColor(QColor(120, 120, 120));
    } else {
        m_engine->page(QStringLiteral("Tables"))->setSystemPage(true);
    }

    if (!m_engine->page(QStringLiteral("Order"))) {
        auto *pg = m_engine->createPage(QStringLiteral("Order"));
        pg->setSystemPage(true);
        auto *title = pg->addLabel(QStringLiteral("order_title"), 20, 10, 600, 60,
                                    QStringLiteral("Order"));
        title->setFontSize(36);
        title->setTextColor(Qt::white);
        title->setAlignment(Qt::AlignLeft);

        auto *hint = pg->addLabel(QStringLiteral("order_hint"), 560, 500, 800, 60,
                                   QStringLiteral("Configure this page in the editor (F2)"));
        hint->setFontSize(20);
        hint->setTextColor(QColor(120, 120, 120));
    } else {
        m_engine->page(QStringLiteral("Order"))->setSystemPage(true);
    }

    // Wire up local keypad → PIN entry connections for all pages
    for (const QString &name : m_engine->pageNames()) {
        wirePageKeypad(name);
    }

    // Always start on the Login page.
    m_engine->showPage(QStringLiteral("Login"));
}

void MainWindow::buildDefaultLoginPage()
{
    auto *pg = m_engine->createPage(QStringLiteral("Login"));
    pg->setSystemPage(true);

    // ── Background panel ────────────────────────────────────────────────
    auto *bg = pg->addPanel(QStringLiteral("login_bg"), 0, 0, 1920, 1080);
    bg->setBgColor(QColor(25, 25, 30));
    bg->setCornerRadius(0);

    // ── Title ───────────────────────────────────────────────────────────
    auto *title = pg->addLabel(QStringLiteral("login_title"), 610, 80, 700, 70,
                                QStringLiteral("ViewTouchQt POS"));
    title->setFontSize(42);
    title->setTextColor(Qt::white);

    auto *subtitle = pg->addLabel(QStringLiteral("login_subtitle"), 710, 160, 500, 40,
                                   QStringLiteral("Enter your PIN to begin"));
    subtitle->setFontSize(20);
    subtitle->setTextColor(QColor(150, 150, 150));

    // ── PIN Entry field ─────────────────────────────────────────────────
    auto *pin = pg->addPinEntry(QStringLiteral("login_pin"), 710, 230, 500, 70);
    pin->setLabel(QStringLiteral("Enter PIN"));
    pin->setFontSize(32);
    pin->setMaxLength(8);

    // ── Keypad (3×4 grid + bottom row) ──────────────────────────────────
    constexpr qreal kpadX = 710;
    constexpr qreal kpadY = 320;
    constexpr qreal btnW  = 150;
    constexpr qreal btnH  = 80;
    constexpr qreal gap   = 10;

    const QString digits[] = {
        QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"),
        QStringLiteral("4"), QStringLiteral("5"), QStringLiteral("6"),
        QStringLiteral("7"), QStringLiteral("8"), QStringLiteral("9"),
    };

    for (int i = 0; i < 9; ++i) {
        int row = i / 3;
        int col = i % 3;
        qreal x = kpadX + col * (btnW + gap);
        qreal y = kpadY + row * (btnH + gap);
        QString id = QStringLiteral("kpd_%1").arg(digits[i]);
        auto *k = pg->addKeypadButton(id, x, y, btnW, btnH, digits[i]);
        k->setBgColor(QColor(55, 55, 60));
    }

    // Bottom row: Clear, 0, Back
    qreal bottomY = kpadY + 3 * (btnH + gap);
    auto *kClear = pg->addKeypadButton(QStringLiteral("kpd_clear"),
                                       kpadX, bottomY, btnW, btnH,
                                       QStringLiteral("Clear"));
    kClear->setKeyValue(QStringLiteral("CLEAR"));
    kClear->setBgColor(QColor(80, 40, 40));
    kClear->setFontSize(20);

    auto *k0 = pg->addKeypadButton(QStringLiteral("kpd_0"),
                                   kpadX + (btnW + gap), bottomY, btnW, btnH,
                                   QStringLiteral("0"));
    k0->setBgColor(QColor(55, 55, 60));

    auto *kBack = pg->addKeypadButton(QStringLiteral("kpd_back"),
                                      kpadX + 2 * (btnW + gap), bottomY, btnW, btnH,
                                      QStringLiteral("←"));
    kBack->setKeyValue(QStringLiteral("BACK"));
    kBack->setBgColor(QColor(70, 60, 40));
    kBack->setFontSize(24);

    // ── Action buttons ──────────────────────────────────────────────────
    qreal actY = bottomY + btnH + 30;
    qreal actW = (3 * btnW + 2 * gap) / 3.0;

    auto *btnLogin = pg->addActionButton(QStringLiteral("act_login"),
                                         kpadX, actY, actW, 70,
                                         QStringLiteral("Login"),
                                         ActionType::Login);
    btnLogin->setBgColor(QColor(0, 120, 215));
    btnLogin->setFontSize(24);

    auto *btnDineIn = pg->addActionButton(QStringLiteral("act_dinein"),
                                          kpadX + actW + gap, actY, actW, 70,
                                          QStringLiteral("Dine-In"),
                                          ActionType::DineIn);
    btnDineIn->setBgColor(QColor(40, 140, 80));
    btnDineIn->setFontSize(24);

    auto *btnToGo = pg->addActionButton(QStringLiteral("act_togo"),
                                        kpadX + 2 * (actW + gap), actY, actW, 70,
                                        QStringLiteral("To-Go"),
                                        ActionType::ToGo);
    btnToGo->setBgColor(QColor(180, 100, 30));
    btnToGo->setFontSize(24);

    // ── Status label ────────────────────────────────────────────────────
    auto *status = pg->addLabel(QStringLiteral("login_status"), 710, actY + 90, 500, 40,
                                 QStringLiteral(""));
    status->setFontSize(18);
    status->setTextColor(QColor(255, 100, 100));

    // ── Edit hint ───────────────────────────────────────────────────────
    auto *hint = pg->addLabel(QStringLiteral("login_hint"), 1400, 1030, 500, 40,
                               QStringLiteral("Press F2 to edit layout"));
    hint->setFontSize(16);
    hint->setTextColor(QColor(80, 80, 80));
    hint->setAlignment(Qt::AlignRight);
}

void MainWindow::wirePageKeypad(const QString &pageName)
{
    auto *pg = m_engine->page(pageName);
    if (!pg) return;

    // Find the PinEntry element on this page
    PinEntryElement *pinEntry = nullptr;
    for (UiElement *elem : pg->elements()) {
        if (elem->elementType() == ElementType::PinEntry) {
            pinEntry = static_cast<PinEntryElement *>(elem);
            break;
        }
    }
    if (!pinEntry) return;

    // Connect each KeypadButton to the PinEntry
    for (UiElement *elem : pg->elements()) {
        if (elem->elementType() == ElementType::KeypadButton) {
            auto *kpd = static_cast<KeypadButtonElement *>(elem);
            // Disconnect any previous connection to avoid duplicates
            disconnect(kpd, &KeypadButtonElement::keyPressed, nullptr, nullptr);
            // Reconnect to this page's keypadPressed signal (which we now handle directly)
            connect(kpd, &KeypadButtonElement::keyPressed, pinEntry,
                    [pinEntry](const QString &value) {
                        if (value == QStringLiteral("BACK")) {
                            pinEntry->backspace();
                        } else if (value == QStringLiteral("CLEAR")) {
                            pinEntry->clearPin();
                        } else {
                            for (QChar ch : value)
                                pinEntry->appendChar(ch);
                        }
                    });
        }
    }
}

void MainWindow::handleAction(const QString &pageName, ActionType action, const QString &targetPage)
{
    // Find the PinEntry on the source page to validate
    auto *pg = m_engine->page(pageName);
    if (!pg) return;

    PinEntryElement *pinEntry = nullptr;
    for (UiElement *elem : pg->elements()) {
        if (elem->elementType() == ElementType::PinEntry) {
            pinEntry = static_cast<PinEntryElement *>(elem);
            break;
        }
    }

    // If there's a PinEntry on this page, require a non-empty PIN
    if (pinEntry && pinEntry->pinText().isEmpty()) {
        // Show feedback: find the status label
        if (auto *statusElem = pg->element(QStringLiteral("login_status"))) {
            statusElem->setLabel(QStringLiteral("Please enter your PIN first"));
        }
        return;
    }

    // Clear the status message
    if (auto *statusElem = pg->element(QStringLiteral("login_status"))) {
        statusElem->setLabel(QString());
    }

    // Clear the PIN for next login
    if (pinEntry)
        pinEntry->clearPin();

    // Navigate based on action type
    switch (action) {
    case ActionType::Login:
        m_engine->showPage(QStringLiteral("Tables"));
        break;
    case ActionType::DineIn:
    case ActionType::ToGo:
        m_engine->showPage(QStringLiteral("Order"));
        break;
    case ActionType::Logout:
        m_engine->showPage(QStringLiteral("Login"));
        break;
    case ActionType::Navigation:
        if (!targetPage.isEmpty() && m_engine->page(targetPage))
            m_engine->showPage(targetPage);
        else
            qWarning() << "[action] Navigation target page not found:" << targetPage;
        break;
    }
}

// ── Layout sync from server ─────────────────────────────────────────────────

void MainWindow::applyLayoutFromNetwork(const QByteArray &layoutJson)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(layoutJson, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "[sync] Layout JSON parse error:" << err.errorString();
        return;
    }
    if (!doc.isObject()) {
        qWarning() << "[sync] Layout root is not a JSON object";
        return;
    }

    // If in edit mode, leave it first so the overlay state is clean.
    if (m_editor->isEditMode())
        m_editor->setEditMode(false);

    if (LayoutSerializer::deserialize(m_engine, doc.object())) {
        ensureSystemPages();
        qInfo() << "[sync] Layout applied from server,"
                << m_engine->pageNames().size() << "pages";
    } else {
        qWarning() << "[sync] Failed to deserialize layout from server";
    }
}

} // namespace vt
