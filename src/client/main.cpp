// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/main.cpp — Entry point for the fullscreen POS terminal client.

#include "MainWindow.h"
#include "PosClient.h"
#include "Protocol.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("vt_client");
    QApplication::setApplicationVersion("0.1.0");

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

    return app.exec();
}
