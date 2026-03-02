// SPDX-License-Identifier: GPL-3.0-or-later
// src/server/main.cpp — Entry point for the headless POS server.

#include "PosServer.h"
#include "Protocol.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QHostAddress>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("vt_server");
    QCoreApplication::setApplicationVersion("0.1.0");

    // ── CLI options ─────────────────────────────────────────────────────────
    QCommandLineParser parser;
    parser.setApplicationDescription("ViewTouchQt POS Server");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOpt(
        QStringList() << "p" << "port",
        "TCP port to listen on (default: 12000).",
        "port",
        QString::number(vt::kDefaultPort));
    parser.addOption(portOpt);

    QCommandLineOption bindOpt(
        QStringList() << "b" << "bind",
        "Address to bind to (default: 0.0.0.0).",
        "address",
        "0.0.0.0");
    parser.addOption(bindOpt);

    parser.process(app);

    const quint16 port = static_cast<quint16>(parser.value(portOpt).toUInt());
    const QHostAddress bindAddr(parser.value(bindOpt));

    // ── Start server ────────────────────────────────────────────────────────
    vt::PosServer server;
    if (!server.startListening(bindAddr, port))
        return 1;

    return app.exec();
}
