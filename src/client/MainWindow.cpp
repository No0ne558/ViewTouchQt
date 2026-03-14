// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/MainWindow.cpp

#include "MainWindow.h"
#include "editor/PropertyDialog.h"
#include "editor/LayoutSerializer.h"
#include "editor/PageManagerDialog.h"
#include "layout/UiElement.h"
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
#include <QTemporaryFile>
#include <QFileInfo>
#include <QFile>
#include <unistd.h>

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

    // Keypad and action button features removed; no connections.

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
            QJsonObject root = LayoutSerializer::serialize(m_engine);
            QJsonDocument doc(root);
            // Try normal save first; if it fails, attempt system save helper.
            if (!LayoutSerializer::saveToFile(m_engine, path)) {
                if (!saveLayoutWithElevation(doc, path)) {
                    qWarning() << "[main] Failed to save layout to" << path;
                }
            }

            // Notify (host wiring broadcasts this to clients)
            emit layoutChanged(doc.toJson(QJsonDocument::Compact));
        }
    });

    // ── Load layout or build default (single Login page) ────────────────
    if (!loadLayoutIfExists())
        buildDefaultLoginPage();

    // Ensure only the Login system page exists as the default starting page.
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

bool MainWindow::saveLayoutWithElevation(const QJsonDocument &doc, const QString &targetPath)
{
    // Write to a temporary file in user space first.
    QTemporaryFile tmp;
    tmp.setAutoRemove(false);
    if (!tmp.open()) {
        qWarning() << "[main] Cannot create temporary file for layout save";
        return false;
    }
    QByteArray data = doc.toJson(QJsonDocument::Indented);
    tmp.write(data);
    tmp.flush();
    tmp.close();

    // Directly copy the temporary file into place. This function now
    // assumes the process has the necessary permissions (e.g., is run as
    // root). It performs an atomic-like replace by removing any existing
    // target first and then copying the temp file.
    if (QFile::exists(targetPath)) {
        if (!QFile::remove(targetPath)) {
            qWarning() << "[main] Failed to remove existing target" << targetPath;
            QFile::remove(tmp.fileName());
            return false;
        }
    }

    if (!QFile::copy(tmp.fileName(), targetPath)) {
        qWarning() << "[main] Failed to copy temp file to" << targetPath;
        QFile::remove(tmp.fileName());
        return false;
    }

    // Ensure permissions are rw-r--r-- (0644).
    QFile::Permissions perms = QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther;
    QFile target(targetPath);
    target.setPermissions(perms);

    QFile::remove(tmp.fileName());
    qInfo() << "[main] Layout saved to" << targetPath;
    return true;
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
    PropertyDialog dlg(elem, this);  // parented to MainWindow so stacking is predictable
    dlg.setWindowFlag(Qt::WindowStaysOnTopHint, true);
    dlg.setPageNames(m_engine->pageNames());
    if (dlg.exec() == QDialog::Accepted) {
        UiElement *target = elem;

        // If the user changed the element type, replace it on the page
                if (dlg.typeChanged()) {
            auto *page = m_engine->activePage();
            if (page) {
                // Deselect BEFORE replacing to avoid use-after-free on
                // decoration items that are children of the old element.
                m_editor->deselectAll();

                auto *replacement = page->replaceElementType(
                    elem->elementId(), dlg.newType());
                if (replacement) {
                    target = replacement;

                    // Apply type-specific properties from the dialog
                            // Only Button-specific properties remain.
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
    // Always use the system data directory for layout storage.
    const QString systemDatPath = QStringLiteral("/opt/viewtouch/dat");
    QDir datDir(systemDatPath);
    if (!datDir.exists())
        QDir().mkpath(systemDatPath);
    return datDir.filePath(QStringLiteral("layout.json"));
}

bool MainWindow::loadLayoutIfExists()
{
    // Load the layout directly from the system data directory.
    QString systemLayout = defaultLayoutPath();
    if (!QFile::exists(systemLayout))
        return false;
    return LayoutSerializer::loadFromFile(m_engine, systemLayout);
}

// ── System pages ────────────────────────────────────────────────────────────

void MainWindow::ensureSystemPages()
{
    // Ensure only the Login page exists and is shown.  Clear any other pages
    // that may have been loaded from an existing layout.
    if (m_engine->pageNames().size() > 1) {
        // Remove all existing pages and recreate a single Login page.
        m_engine->clearAll();
    }

    if (!m_engine->page(QStringLiteral("Login"))) {
        // Create a minimal, empty Login page (no inputs or buttons).
        auto *pg = m_engine->createPage(QStringLiteral("Login"));
        pg->setSystemPage(true);
    } else {
        m_engine->page(QStringLiteral("Login"))->setSystemPage(true);
    }

    // Show Login page only.
    m_engine->showPage(QStringLiteral("Login"));
}
void MainWindow::buildDefaultLoginPage()
{
    // Create a minimal Login page with no inputs or buttons.
    // Clear any existing pages to ensure this is the only page.
    m_engine->clearAll();
    auto *pg = m_engine->createPage(QStringLiteral("Login"));
    if (pg) pg->setSystemPage(true);
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
