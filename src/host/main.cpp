// SPDX-License-Identifier: GPL-3.0-or-later
// src/host/main.cpp — Combined host: starts the POS server AND a main client UI.
//
// Usage:
//   ./vt_host [--port 9100]
//
// This is the primary entry point for the host device.  It:
//   1. Starts PosServer on the requested port (default 9100).
//   2. Opens a fullscreen MainWindow (the "main terminal") that connects
//      to the local server via loopback.
//
// Remote terminal_client instances connect to the same server over the network.

#include "MainWindow.h"
#include "PosClient.h"
#include "PosServer.h"
#include "Protocol.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QHostAddress>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("vt_host");
    QApplication::setApplicationVersion("0.1.0");

    // ── CLI options ─────────────────────────────────────────────────────────
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "ViewTouchQt POS Host — Server + Main Terminal");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOpt(
        QStringList() << "p" << "port",
        "TCP port for the server (default: 12000).",
        "port",
        QString::number(vt::kDefaultPort));
    parser.addOption(portOpt);

    QCommandLineOption bindOpt(
        QStringList() << "b" << "bind",
        "Address to bind the server to (default: 0.0.0.0).",
        "address",
        "0.0.0.0");
    parser.addOption(bindOpt);

    parser.process(app);

    const quint16       port     = static_cast<quint16>(parser.value(portOpt).toUInt());
    const QHostAddress  bindAddr(parser.value(bindOpt));

    // ── 1. Start the server ─────────────────────────────────────────────────
    vt::PosServer server;
    if (!server.startListening(bindAddr, port))
        return 1;

    // ── 2. Start the local "main client" ────────────────────────────────────
    vt::PosClient client;
    vt::MainWindow window(&client);
    window.show();

    // When the host edits and saves the layout, push it to the server so
    // all remote clients receive it automatically.
    QObject::connect(&window, &vt::MainWindow::layoutChanged,
                     &server, &vt::PosServer::setCurrentLayout);

    // Connect to the local server once the event loop is running so the
    // server socket is fully ready.
    QTimer::singleShot(0, &client, [&client, port]() {
        client.connectToServer(QStringLiteral("127.0.0.1"), port);
    });

    return app.exec();
}
