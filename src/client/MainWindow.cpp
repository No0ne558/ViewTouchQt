// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/MainWindow.cpp

#include "MainWindow.h"
#include "editor/PropertyDialog.h"
#include "editor/LayoutSerializer.h"
#include "editor/PageManagerDialog.h"
#include "layout/PinEntryElement.h"
#include "layout/KeypadButtonElement.h"
#include "layout/ActionButtonElement.h"
#include "displays/DisplayManager.h"
#include "printing/CupsPrinter.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QProcess>
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

    // ── Display manager ─────────────────────────────────────────────────
    m_displayMgr = new DisplayManager(this);
    m_displayMgr->loadFromFile(DisplayManager::defaultFilePath());

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

    // Determine the best available screen size.  QScreen may return a
    // tiny or zero geometry on remote X servers like XServer XSDL, so
    // fall back to the design resolution when the reported size is too
    // small (< 640 px in either axis).
    QRect screenGeo;
    if (QScreen *scr = screen()) {
        screenGeo = scr->geometry();
    }
    if (screenGeo.width() < 640 || screenGeo.height() < 480) {
        screenGeo = QRect(0, 0, static_cast<int>(kDesignW), static_cast<int>(kDesignH));
    }
    setGeometry(screenGeo);
    resize(screenGeo.size());

    // Defer the initial layout broadcast so that main.cpp has time to
    // wire layoutChanged → PosServer::setCurrentLayout before it fires.
    QTimer::singleShot(0, this, [this]() {
        QJsonObject root = LayoutSerializer::serialize(m_engine);
        QJsonDocument doc(root);
        emit layoutChanged(doc.toJson(QJsonDocument::Compact));

        // Launch remote clients for all active displays once the server is ready.
        launchAllActiveDisplays();
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

        QRect geo;
        if (QScreen *scr = screen())
            geo = scr->availableGeometry();
        if (geo.width() < 640 || geo.height() < 480)
            geo = QRect(0, 0, static_cast<int>(kDesignW), static_cast<int>(kDesignH));

        setGeometry(geo);
        resize(geo.size());
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

    // ── Displays page ───────────────────────────────────────────────────
    if (!m_engine->page(QStringLiteral("Displays"))) {
        buildDisplaysPage();
    } else {
        m_engine->page(QStringLiteral("Displays"))->setSystemPage(true);
    }

    // ── DisplayEdit page ────────────────────────────────────────────────
    if (!m_engine->page(QStringLiteral("DisplayEdit"))) {
        buildDisplayEditPage();
    } else {
        m_engine->page(QStringLiteral("DisplayEdit"))->setSystemPage(true);
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

    // Collect all PinEntry elements on this page.
    QList<PinEntryElement *> pinEntries;
    for (UiElement *elem : pg->elements()) {
        if (elem->elementType() == ElementType::PinEntry)
            pinEntries.append(static_cast<PinEntryElement *>(elem));
    }
    if (pinEntries.isEmpty()) return;

    // Connect each KeypadButton to the focused PinEntry (or the first one).
    for (UiElement *elem : pg->elements()) {
        if (elem->elementType() == ElementType::KeypadButton) {
            auto *kpd = static_cast<KeypadButtonElement *>(elem);
            // Disconnect any previous connection to avoid duplicates
            disconnect(kpd, &KeypadButtonElement::keyPressed, nullptr, nullptr);
            connect(kpd, &KeypadButtonElement::keyPressed, this,
                    [pinEntries](const QString &value) {
                        // Find the PinEntry that has focus; default to first.
                        PinEntryElement *target = pinEntries.first();
                        for (auto *pe : pinEntries) {
                            if (pe->hasFocus()) { target = pe; break; }
                        }
                        if (value == QStringLiteral("BACK")) {
                            target->backspace();
                        } else if (value == QStringLiteral("CLEAR")) {
                            target->clearPin();
                        } else {
                            for (QChar ch : value)
                                target->appendChar(ch);
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

    // Login-page PIN validation / clearing (only applies to Login actions)
    if (action == ActionType::Login) {
        PinEntryElement *pinEntry = nullptr;
        for (UiElement *elem : pg->elements()) {
            if (elem->elementType() == ElementType::PinEntry) {
                pinEntry = static_cast<PinEntryElement *>(elem);
                break;
            }
        }

        if (pinEntry && pinEntry->pinText().isEmpty()) {
            if (auto *statusElem = pg->element(QStringLiteral("login_status")))
                statusElem->setLabel(QStringLiteral("Please enter your PIN first"));
            return;
        }

        if (auto *statusElem = pg->element(QStringLiteral("login_status")))
            statusElem->setLabel(QString());

        if (pinEntry)
            pinEntry->clearPin();
    }

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
    case ActionType::ShowDisplays:
        refreshDisplayList();
        m_engine->showPage(QStringLiteral("Displays"));
        break;
    case ActionType::AddDisplay:
        handleAddDisplay();
        break;
    case ActionType::EditDisplay:
        handleEditDisplay();
        break;
    case ActionType::RemoveDisplay:
        handleRemoveDisplay();
        break;
    case ActionType::ToggleDisplay:
        handleToggleDisplay();
        break;
    case ActionType::TestPrinter:
        handleTestPrinter();
        break;
    case ActionType::DisplayDone:
        handleDisplayDone();
        break;
    }
}

// ── Displays page ───────────────────────────────────────────────────────────

void MainWindow::buildDisplaysPage()
{
    auto *pg = m_engine->createPage(QStringLiteral("Displays"));
    pg->setSystemPage(true);

    // Background
    auto *bg = pg->addPanel(QStringLiteral("disp_bg"), 0, 0, 1920, 1080);
    bg->setBgColor(QColor(25, 25, 30));
    bg->setCornerRadius(0);

    // Title
    auto *title = pg->addLabel(QStringLiteral("disp_title"), 40, 30, 600, 60,
                                QStringLiteral("Displays"));
    title->setFontSize(40);
    title->setTextColor(Qt::white);

    // Display list area — labels will be populated dynamically
    // Placeholder hint
    auto *hint = pg->addLabel(QStringLiteral("disp_hint"), 40, 110, 1200, 40,
                               QStringLiteral("No displays configured yet."));
    hint->setFontSize(18);
    hint->setTextColor(QColor(150, 150, 150));

    // We'll create up to 10 row labels for the display list
    constexpr qreal listX = 40;
    constexpr qreal listY = 160;
    constexpr qreal rowH  = 60;
    constexpr qreal rowGap = 10;

    for (int i = 0; i < 10; ++i) {
        qreal y = listY + i * (rowH + rowGap);
        QString id = QStringLiteral("disp_row_%1").arg(i);
        auto *row = pg->addButton(id, listX, y, 1200, rowH, QString());
        row->setBgColor(QColor(45, 45, 50));
        row->setTextColor(Qt::white);
        row->setFontSize(20);
        row->setCornerRadius(6);
        row->setVisible(false);

        // Wire selection handler once — index is fixed per row button.
        connect(row, &ButtonElement::clicked, this, [this, i]() {
            if (i < m_displayMgr->count()) {
                m_selectedDisplayIdx = i;
                refreshDisplayList();
            }
        });
    }

    // Action buttons — right column
    constexpr qreal btnX = 1350;
    constexpr qreal btnW = 500;
    constexpr qreal btnH = 80;
    constexpr qreal btnGap = 20;

    auto *addBtn = pg->addActionButton(QStringLiteral("disp_add"), btnX, 160, btnW, btnH,
                                        QStringLiteral("Add Display"), ActionType::AddDisplay);
    addBtn->setBgColor(QColor(40, 120, 40));
    addBtn->setTextColor(Qt::white);
    addBtn->setFontSize(24);

    auto *editBtn = pg->addActionButton(QStringLiteral("disp_edit"), btnX, 160 + (btnH + btnGap), btnW, btnH,
                                         QStringLiteral("Edit Display"), ActionType::EditDisplay);
    editBtn->setBgColor(QColor(50, 100, 180));
    editBtn->setTextColor(Qt::white);
    editBtn->setFontSize(24);

    auto *removeBtn = pg->addActionButton(QStringLiteral("disp_remove"), btnX, 160 + 2*(btnH + btnGap), btnW, btnH,
                                           QStringLiteral("Remove Display"), ActionType::RemoveDisplay);
    removeBtn->setBgColor(QColor(180, 40, 40));
    removeBtn->setTextColor(Qt::white);
    removeBtn->setFontSize(24);

    auto *toggleBtn = pg->addActionButton(QStringLiteral("disp_toggle"), btnX, 160 + 3*(btnH + btnGap), btnW, btnH,
                                           QStringLiteral("Activate / Deactivate"), ActionType::ToggleDisplay);
    toggleBtn->setBgColor(QColor(160, 120, 20));
    toggleBtn->setTextColor(Qt::white);
    toggleBtn->setFontSize(24);

    // Back button
    auto *backBtn = pg->addActionButton(QStringLiteral("disp_back"), btnX, 160 + 5*(btnH + btnGap), btnW, btnH,
                                         QStringLiteral("← Back"), ActionType::Navigation);
    backBtn->setTargetPage(QStringLiteral("Tables"));
    backBtn->setBgColor(QColor(80, 80, 80));
    backBtn->setTextColor(Qt::white);
    backBtn->setFontSize(24);

    // Status label
    auto *status = pg->addLabel(QStringLiteral("disp_status"), 40, 900, 1200, 40, QString());
    status->setFontSize(18);
    status->setTextColor(QColor(200, 200, 100));
}

void MainWindow::buildDisplayEditPage()
{
    auto *pg = m_engine->createPage(QStringLiteral("DisplayEdit"));
    pg->setSystemPage(true);

    // Background
    auto *bg = pg->addPanel(QStringLiteral("dedit_bg"), 0, 0, 1920, 1080);
    bg->setBgColor(QColor(25, 25, 30));
    bg->setCornerRadius(0);

    // Title
    auto *title = pg->addLabel(QStringLiteral("dedit_title"), 40, 30, 800, 60,
                                QStringLiteral("Edit Display"));
    title->setFontSize(40);
    title->setTextColor(Qt::white);

    // Form fields — centered column
    constexpr qreal formX = 460;
    constexpr qreal labelW = 340;
    constexpr qreal fieldX = 810;
    constexpr qreal fieldW = 600;
    constexpr qreal rowH = 60;
    constexpr qreal rowGap = 30;
    qreal y = 160;

    // Display Name
    auto *nameLabel = pg->addLabel(QStringLiteral("dedit_name_lbl"), formX, y, labelW, rowH,
                                    QStringLiteral("Display Name:"));
    nameLabel->setFontSize(26);
    nameLabel->setTextColor(Qt::white);
    nameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto *nameField = pg->addPinEntry(QStringLiteral("dedit_name"), fieldX, y, fieldW, rowH);
    nameField->setLabel(QStringLiteral("e.g. Bar 1"));
    nameField->setMasked(false);
    nameField->setMaxLength(30);
    nameField->setFontSize(24);

    y += rowH + rowGap;

    // Display IP Address
    auto *ipLabel = pg->addLabel(QStringLiteral("dedit_ip_lbl"), formX, y, labelW, rowH,
                                  QStringLiteral("Display IP Address:"));
    ipLabel->setFontSize(26);
    ipLabel->setTextColor(Qt::white);
    ipLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto *ipField = pg->addPinEntry(QStringLiteral("dedit_ip"), fieldX, y, fieldW, rowH);
    ipField->setLabel(QStringLiteral("e.g. 192.168.1.72"));
    ipField->setMasked(false);
    ipField->setMaxLength(45);
    ipField->setFontSize(24);

    y += rowH + rowGap;

    // Printer (label — the actual selection is done via a button that cycles)
    auto *printerLabel = pg->addLabel(QStringLiteral("dedit_printer_lbl"), formX, y, labelW, rowH,
                                       QStringLiteral("Printer:"));
    printerLabel->setFontSize(26);
    printerLabel->setTextColor(Qt::white);
    printerLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Printer selection button — tapping cycles through available CUPS printers
    auto *printerBtn = pg->addButton(QStringLiteral("dedit_printer"), fieldX, y, fieldW, rowH,
                                      QStringLiteral("(none)"));
    printerBtn->setBgColor(QColor(55, 55, 65));
    printerBtn->setTextColor(Qt::white);
    printerBtn->setFontSize(22);

    y += rowH + rowGap;

    // Test Printer button
    auto *testBtn = pg->addActionButton(QStringLiteral("dedit_test"), fieldX, y, fieldW, rowH,
                                         QStringLiteral("Test Printer"), ActionType::TestPrinter);
    testBtn->setBgColor(QColor(50, 100, 180));
    testBtn->setTextColor(Qt::white);
    testBtn->setFontSize(24);

    y += rowH + rowGap + 20;

    // Done button
    auto *doneBtn = pg->addActionButton(QStringLiteral("dedit_done"), fieldX, y, fieldW, rowH,
                                         QStringLiteral("Done"), ActionType::DisplayDone);
    doneBtn->setBgColor(QColor(40, 120, 40));
    doneBtn->setTextColor(Qt::white);
    doneBtn->setFontSize(28);

    // Status label
    auto *status = pg->addLabel(QStringLiteral("dedit_status"), 40, 900, 1200, 40, QString());
    status->setFontSize(18);
    status->setTextColor(QColor(200, 200, 100));

    // Keyboard for text input (full QWERTY + numbers + special)
    buildDisplayEditKeyboard(pg);
}

void MainWindow::buildDisplayEditKeyboard(PageWidget *pg)
{
    // Compact on-screen QWERTY keyboard placed at the bottom of the page.
    // Each key is a KeypadButton; letters arrive via the keypad wiring to
    // whichever PinEntry currently has "focus" (the page has two: name and ip).

    constexpr qreal kbdY = 580;
    constexpr qreal keyW = 110;
    constexpr qreal keyH = 70;
    constexpr qreal gap  = 6;

    // Row 1: numbers + period + dash
    const QStringList row1 = {
        QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"),
        QStringLiteral("4"), QStringLiteral("5"), QStringLiteral("6"),
        QStringLiteral("7"), QStringLiteral("8"), QStringLiteral("9"),
        QStringLiteral("0"), QStringLiteral("."), QStringLiteral("-"),
    };
    for (int i = 0; i < row1.size(); ++i) {
        qreal x = 100 + i * (keyW + gap);
        QString id = QStringLiteral("dk_r1_%1").arg(i);
        auto *k = pg->addKeypadButton(id, x, kbdY, keyW, keyH, row1[i]);
        k->setBgColor(QColor(55, 55, 60));
        k->setTextColor(Qt::white);
        k->setFontSize(22);
    }

    // Row 2: QWERTYUIOP
    const QStringList row2 = {
        QStringLiteral("Q"), QStringLiteral("W"), QStringLiteral("E"),
        QStringLiteral("R"), QStringLiteral("T"), QStringLiteral("Y"),
        QStringLiteral("U"), QStringLiteral("I"), QStringLiteral("O"),
        QStringLiteral("P"),
    };
    qreal row2Y = kbdY + keyH + gap;
    for (int i = 0; i < row2.size(); ++i) {
        qreal x = 100 + i * (keyW + gap);
        QString id = QStringLiteral("dk_r2_%1").arg(i);
        auto *k = pg->addKeypadButton(id, x, row2Y, keyW, keyH, row2[i]);
        k->setBgColor(QColor(55, 55, 60));
        k->setTextColor(Qt::white);
        k->setFontSize(22);
    }

    // Row 3: ASDFGHJKL
    const QStringList row3 = {
        QStringLiteral("A"), QStringLiteral("S"), QStringLiteral("D"),
        QStringLiteral("F"), QStringLiteral("G"), QStringLiteral("H"),
        QStringLiteral("J"), QStringLiteral("K"), QStringLiteral("L"),
    };
    qreal row3Y = row2Y + keyH + gap;
    for (int i = 0; i < row3.size(); ++i) {
        qreal x = 155 + i * (keyW + gap);
        QString id = QStringLiteral("dk_r3_%1").arg(i);
        auto *k = pg->addKeypadButton(id, x, row3Y, keyW, keyH, row3[i]);
        k->setBgColor(QColor(55, 55, 60));
        k->setTextColor(Qt::white);
        k->setFontSize(22);
    }

    // Row 4: ZXCVBNM + BACK
    const QStringList row4 = {
        QStringLiteral("Z"), QStringLiteral("X"), QStringLiteral("C"),
        QStringLiteral("V"), QStringLiteral("B"), QStringLiteral("N"),
        QStringLiteral("M"),
    };
    qreal row4Y = row3Y + keyH + gap;
    for (int i = 0; i < row4.size(); ++i) {
        qreal x = 210 + i * (keyW + gap);
        QString id = QStringLiteral("dk_r4_%1").arg(i);
        auto *k = pg->addKeypadButton(id, x, row4Y, keyW, keyH, row4[i]);
        k->setBgColor(QColor(55, 55, 60));
        k->setTextColor(Qt::white);
        k->setFontSize(22);
    }

    // Backspace + Space + Clear on bottom row
    qreal row5Y = row4Y + keyH + gap;

    auto *backKey = pg->addKeypadButton(QStringLiteral("dk_back"), 210, row5Y, 200, keyH,
                                         QStringLiteral("← Back"));
    backKey->setKeyValue(QStringLiteral("BACK"));
    backKey->setBgColor(QColor(120, 50, 50));
    backKey->setTextColor(Qt::white);
    backKey->setFontSize(20);

    auto *spaceKey = pg->addKeypadButton(QStringLiteral("dk_space"), 420, row5Y, 400, keyH,
                                          QStringLiteral("Space"));
    spaceKey->setKeyValue(QStringLiteral(" "));
    spaceKey->setBgColor(QColor(65, 65, 70));
    spaceKey->setTextColor(Qt::white);
    spaceKey->setFontSize(20);

    auto *clearKey = pg->addKeypadButton(QStringLiteral("dk_clear"), 830, row5Y, 200, keyH,
                                          QStringLiteral("Clear"));
    clearKey->setKeyValue(QStringLiteral("CLEAR"));
    clearKey->setBgColor(QColor(120, 80, 20));
    clearKey->setTextColor(Qt::white);
    clearKey->setFontSize(20);
}

// ── Display management handlers ─────────────────────────────────────────────

void MainWindow::refreshDisplayList()
{
    auto *pg = m_engine->page(QStringLiteral("Displays"));
    if (!pg) return;

    const auto &displays = m_displayMgr->displays();

    // Update hint visibility
    if (auto *hint = pg->element(QStringLiteral("disp_hint")))
        hint->setVisible(displays.isEmpty());

    // Update row buttons
    for (int i = 0; i < 10; ++i) {
        QString id = QStringLiteral("disp_row_%1").arg(i);
        auto *row = pg->element(id);
        if (!row) continue;

        if (i < displays.size()) {
            const auto &d = displays.at(i);
            QString text = QStringLiteral("%1  |  %2  |  %3  |  %4")
                .arg(d.name, -15)
                .arg(d.ipAddress, -18)
                .arg(d.printer.isEmpty() ? QStringLiteral("(no printer)") : d.printer, -15)
                .arg(d.active ? QStringLiteral("ACTIVE") : QStringLiteral("INACTIVE"));
            row->setLabel(text);

            auto *btn = static_cast<ButtonElement *>(row);
            btn->setBgColor(i == m_selectedDisplayIdx ? QColor(70, 70, 120) : QColor(45, 45, 50));
            row->setVisible(true);
        } else {
            row->setVisible(false);
        }
    }

    // Row click handlers are wired once in buildDisplaysPage().
    // No disconnect/reconnect needed here.
}

void MainWindow::handleAddDisplay()
{
    auto cfg = DisplayConfig::create(
        QStringLiteral("Display %1").arg(m_displayMgr->count() + 1),
        QStringLiteral("192.168.1."));
    m_displayMgr->addDisplay(cfg);
    m_selectedDisplayIdx = m_displayMgr->count() - 1;
    m_displayMgr->saveToFile(DisplayManager::defaultFilePath());
    refreshDisplayList();

    if (auto *status = m_engine->page(QStringLiteral("Displays"))->element(QStringLiteral("disp_status")))
        status->setLabel(QStringLiteral("Display added. Select it and press Edit to configure."));
}

void MainWindow::handleEditDisplay()
{
    if (m_selectedDisplayIdx < 0 || m_selectedDisplayIdx >= m_displayMgr->count()) {
        if (auto *pg = m_engine->page(QStringLiteral("Displays")))
            if (auto *s = pg->element(QStringLiteral("disp_status")))
                s->setLabel(QStringLiteral("Select a display first."));
        return;
    }

    auto *cfg = m_displayMgr->displayAt(m_selectedDisplayIdx);
    if (!cfg) return;
    m_editingDisplayUuid = cfg->uuid;

    // Populate DisplayEdit fields
    auto *editPg = m_engine->page(QStringLiteral("DisplayEdit"));
    if (!editPg) return;

    if (auto *nameField = static_cast<PinEntryElement *>(editPg->element(QStringLiteral("dedit_name")))) {
        nameField->clearPin();
        for (QChar ch : cfg->name)
            nameField->appendChar(ch);
    }
    if (auto *ipField = static_cast<PinEntryElement *>(editPg->element(QStringLiteral("dedit_ip")))) {
        ipField->clearPin();
        for (QChar ch : cfg->ipAddress)
            ipField->appendChar(ch);
    }

    // Set printer button label
    if (auto *printerBtn = editPg->element(QStringLiteral("dedit_printer"))) {
        printerBtn->setLabel(cfg->printer.isEmpty() ? QStringLiteral("(none)") : cfg->printer);
    }

    // Wire printer button to cycle through CUPS printers
    if (auto *printerBtn = static_cast<ButtonElement *>(editPg->element(QStringLiteral("dedit_printer")))) {
        disconnect(printerBtn, &ButtonElement::clicked, nullptr, nullptr);
        connect(printerBtn, &ButtonElement::clicked, this, [this, printerBtn]() {
            QStringList printers = CupsPrinter::availablePrinters();
            if (printers.isEmpty()) {
                printerBtn->setLabel(QStringLiteral("(no printers found)"));
                return;
            }
            // Cycle to next printer
            QString current = printerBtn->label();
            int idx = printers.indexOf(current);
            int next = (idx + 1) % printers.size();
            printerBtn->setLabel(printers.at(next));
        });
    }

    m_engine->showPage(QStringLiteral("DisplayEdit"));
}

void MainWindow::handleRemoveDisplay()
{
    if (m_selectedDisplayIdx < 0 || m_selectedDisplayIdx >= m_displayMgr->count()) {
        if (auto *pg = m_engine->page(QStringLiteral("Displays")))
            if (auto *s = pg->element(QStringLiteral("disp_status")))
                s->setLabel(QStringLiteral("Select a display to remove."));
        return;
    }

    auto *cfg = m_displayMgr->displayAt(m_selectedDisplayIdx);
    if (cfg) {
        QString name = cfg->name;
        QString uuid = cfg->uuid;
        stopDisplayClient(uuid);
        m_displayMgr->removeDisplay(uuid);
        m_displayMgr->saveToFile(DisplayManager::defaultFilePath());
        m_selectedDisplayIdx = -1;
        refreshDisplayList();

        if (auto *pg = m_engine->page(QStringLiteral("Displays")))
            if (auto *s = pg->element(QStringLiteral("disp_status")))
                s->setLabel(QStringLiteral("Removed: ") + name);
    }
}

void MainWindow::handleToggleDisplay()
{
    if (m_selectedDisplayIdx < 0 || m_selectedDisplayIdx >= m_displayMgr->count()) {
        if (auto *pg = m_engine->page(QStringLiteral("Displays")))
            if (auto *s = pg->element(QStringLiteral("disp_status")))
                s->setLabel(QStringLiteral("Select a display to toggle."));
        return;
    }

    auto *cfg = m_displayMgr->displayAt(m_selectedDisplayIdx);
    if (cfg) {
        bool newState = m_displayMgr->toggleActive(cfg->uuid);
        m_displayMgr->saveToFile(DisplayManager::defaultFilePath());

        // Launch or stop the remote client process.
        if (newState && !cfg->ipAddress.isEmpty())
            launchDisplayClient(*cfg);
        else
            stopDisplayClient(cfg->uuid);

        refreshDisplayList();

        if (auto *pg = m_engine->page(QStringLiteral("Displays")))
            if (auto *s = pg->element(QStringLiteral("disp_status")))
                s->setLabel(cfg->name + (newState ? QStringLiteral(" activated") : QStringLiteral(" deactivated")));
    }
}

void MainWindow::handleTestPrinter()
{
    auto *editPg = m_engine->page(QStringLiteral("DisplayEdit"));
    if (!editPg) return;

    QString printer;
    if (auto *printerBtn = editPg->element(QStringLiteral("dedit_printer")))
        printer = printerBtn->label();

    if (printer.isEmpty() || printer.startsWith('(')) {
        if (auto *s = editPg->element(QStringLiteral("dedit_status")))
            s->setLabel(QStringLiteral("Select a printer first."));
        return;
    }

    bool ok = CupsPrinter::printTestPage(printer);
    if (auto *s = editPg->element(QStringLiteral("dedit_status")))
        s->setLabel(ok ? QStringLiteral("Test page sent to ") + printer
                       : QStringLiteral("Print failed — check CUPS."));
}

void MainWindow::handleDisplayDone()
{
    auto *editPg = m_engine->page(QStringLiteral("DisplayEdit"));
    if (!editPg) return;

    auto *cfg = m_displayMgr->display(m_editingDisplayUuid);
    if (!cfg) {
        m_engine->showPage(QStringLiteral("Displays"));
        return;
    }

    // Read fields back
    if (auto *nameField = static_cast<PinEntryElement *>(editPg->element(QStringLiteral("dedit_name"))))
        cfg->name = nameField->pinText();
    if (auto *ipField = static_cast<PinEntryElement *>(editPg->element(QStringLiteral("dedit_ip"))))
        cfg->ipAddress = ipField->pinText();
    if (auto *printerBtn = editPg->element(QStringLiteral("dedit_printer"))) {
        QString lbl = printerBtn->label();
        cfg->printer = (lbl.startsWith('(')) ? QString() : lbl;
    }

    m_displayMgr->updateDisplay(*cfg);
    m_displayMgr->saveToFile(DisplayManager::defaultFilePath());
    m_editingDisplayUuid.clear();

    refreshDisplayList();
    m_engine->showPage(QStringLiteral("Displays"));

    if (auto *s = m_engine->page(QStringLiteral("Displays"))->element(QStringLiteral("disp_status")))
        s->setLabel(QStringLiteral("Display saved."));
}

// ── Remote display process management ───────────────────────────────────────

QString MainWindow::findClientBinary() const
{
    // Look for vt_client next to the running executable (../client/vt_client
    // relative to the host binary, or same directory).
    QDir appDir(QCoreApplication::applicationDirPath());

    // Try sibling client dir (build tree layout: build/src/host/vt_host → build/src/client/vt_client)
    QString sibling = appDir.absoluteFilePath(QStringLiteral("../client/vt_client"));
    if (QFile::exists(sibling))
        return sibling;

    // Try same directory
    QString sameDir = appDir.absoluteFilePath(QStringLiteral("vt_client"));
    if (QFile::exists(sameDir))
        return sameDir;

    // Try PATH
    QString inPath = QStandardPaths::findExecutable(QStringLiteral("vt_client"));
    if (!inPath.isEmpty())
        return inPath;

    return {};
}

QString MainWindow::detectLocalIp() const
{
    // Find the first non-loopback IPv4 address on a running interface.
    const auto allIfaces = QNetworkInterface::allInterfaces();
    for (const auto &iface : allIfaces) {
        if (iface.flags().testFlag(QNetworkInterface::IsLoopBack))
            continue;
        if (!iface.flags().testFlag(QNetworkInterface::IsUp) ||
            !iface.flags().testFlag(QNetworkInterface::IsRunning))
            continue;
        const auto entries = iface.addressEntries();
        for (const auto &entry : entries) {
            QHostAddress addr = entry.ip();
            if (addr.protocol() == QAbstractSocket::IPv4Protocol)
                return addr.toString();
        }
    }
    return QStringLiteral("127.0.0.1");
}

void MainWindow::launchDisplayClient(const DisplayConfig &cfg)
{
    // Don't double-launch.
    if (m_displayProcesses.contains(cfg.uuid)) {
        auto *existing = m_displayProcesses.value(cfg.uuid);
        if (existing->state() != QProcess::NotRunning)
            return;
        // Previous process is dead — clean up and relaunch.
        existing->deleteLater();
        m_displayProcesses.remove(cfg.uuid);
    }

    QString clientBin = findClientBinary();
    if (clientBin.isEmpty()) {
        qWarning() << "[display] Cannot find vt_client binary";
        return;
    }

    QString serverIp = detectLocalIp();

    auto *proc = new QProcess(this);

    // Set DISPLAY to the remote X server and force xcb platform.
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("DISPLAY"), cfg.ipAddress + QStringLiteral(":0"));
    env.insert(QStringLiteral("QT_XCB_NO_XI2"), QStringLiteral("1"));
    proc->setProcessEnvironment(env);

    QStringList args;
    args << QStringLiteral("-platform") << QStringLiteral("xcb")
         << QStringLiteral("-s") << serverIp
         << QStringLiteral("-p") << QString::number(kDefaultPort);

    connect(proc, &QProcess::errorOccurred, this, [this, uuid = cfg.uuid, name = cfg.name](QProcess::ProcessError err) {
        qWarning() << "[display] Process error for" << name << ":" << err;
        if (auto *pg = m_engine->page(QStringLiteral("Displays")))
            if (auto *s = pg->element(QStringLiteral("disp_status")))
                s->setLabel(name + QStringLiteral(" — failed to launch client"));
    });

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, uuid = cfg.uuid, name = cfg.name](int exitCode, QProcess::ExitStatus status) {
        qInfo() << "[display] Client for" << name << "exited:" << exitCode << status;
        if (m_displayProcesses.contains(uuid)) {
            m_displayProcesses.value(uuid)->deleteLater();
            m_displayProcesses.remove(uuid);
        }
    });

    qInfo() << "[display] Launching client for" << cfg.name
            << "on DISPLAY=" << cfg.ipAddress + ":0"
            << "server=" << serverIp;
    proc->start(clientBin, args);
    m_displayProcesses.insert(cfg.uuid, proc);
}

void MainWindow::stopDisplayClient(const QString &uuid)
{
    auto *proc = m_displayProcesses.value(uuid, nullptr);
    if (!proc)
        return;

    if (proc->state() != QProcess::NotRunning) {
        proc->terminate();
        if (!proc->waitForFinished(3000))
            proc->kill();
    }
    proc->deleteLater();
    m_displayProcesses.remove(uuid);
}

void MainWindow::launchAllActiveDisplays()
{
    const auto &displays = m_displayMgr->displays();
    for (const auto &cfg : displays) {
        if (cfg.active && !cfg.ipAddress.isEmpty())
            launchDisplayClient(cfg);
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
