// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/main.cpp — Entry point for the fullscreen POS terminal client.

#include "MainWindow.h"
#include "PosClient.h"
#include "Protocol.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QMouseEvent>
#include <cmath>

// ── Filter out mouse / touch events that carry NaN coordinates ──────────────
// XServer XSDL on Android can produce invalid touch-to-mouse mappings via
// XInput2 that arrive with NaN positions, causing Qt warnings and broken input.
// This filter silently drops those events.
class NanMouseFilter : public QObject {
public:
    using QObject::QObject;
protected:
    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        switch (ev->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove: {
            auto *me = static_cast<QMouseEvent *>(ev);
            const auto pos = me->position();
            if (std::isnan(pos.x()) || std::isnan(pos.y()))
                return true;   // swallow the bad event
            break;
        }
        default:
            break;
        }
        return QObject::eventFilter(obj, ev);
    }
};

int main(int argc, char *argv[])
{
    // Disable XInput2 on xcb to avoid NaN touch coordinates from XServer XSDL.
    // This env var must be set before QApplication construction.
    qputenv("QT_XCB_NO_XI2", "1");

    QApplication app(argc, argv);
    QApplication::setApplicationName("vt_client");
    QApplication::setApplicationVersion("0.1.0");

    // Safety net: filter any remaining NaN mouse events that slip through.
    NanMouseFilter nanFilter;
    app.installEventFilter(&nanFilter);

    // ── CLI options ─────────────────────────────────────────────────────────
    QCommandLineParser parser;
    parser.setApplicationDescription("ViewTouchQt POS Terminal Client");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption serverOpt(
        QStringList() << "s" << "server",
        "Server IP or hostname (default: 127.0.0.1).",
        "address",
        "127.0.0.1");
    parser.addOption(serverOpt);

    QCommandLineOption portOpt(
        QStringList() << "p" << "port",
        "Server port (default: 12000).",
        "port",
        QString::number(vt::kDefaultPort));
    parser.addOption(portOpt);

    parser.process(app);

    const QString host = parser.value(serverOpt);
    const quint16 port = static_cast<quint16>(parser.value(portOpt).toUInt());

    // ── Network client ──────────────────────────────────────────────────────
    vt::PosClient client;
    client.connectToServer(host, port);

    // ── UI ──────────────────────────────────────────────────────────────────
    vt::MainWindow window(&client);
    window.show();

    // Apply layout updates pushed from the server.
    QObject::connect(&client, &vt::PosClient::layoutSyncReceived,
                     &window, &vt::MainWindow::applyLayoutFromNetwork);

    return app.exec();
}
