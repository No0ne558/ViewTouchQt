// SPDX-License-Identifier: GPL-3.0-or-later
// src/server/PosServer.cpp

#include "PosServer.h"

#include <QDebug>
#include <QTcpSocket>

namespace vt {

PosServer::PosServer(QObject *parent)
    : QTcpServer(parent)
{
}

bool PosServer::startListening(const QHostAddress &address, quint16 port)
{
    if (!listen(address, port)) {
        qCritical() << "[server] Failed to listen on"
                     << address.toString() << ":" << port
                     << "-" << errorString();
        return false;
    }

    qInfo() << "[server] Listening on"
            << serverAddress().toString() << ":" << serverPort();
    return true;
}

void PosServer::incomingConnection(qintptr socketDescriptor)
{
    auto *socket = new QTcpSocket(this);
    if (!socket->setSocketDescriptor(socketDescriptor)) {
        qWarning() << "[server] Failed to set socket descriptor";
        delete socket;
        return;
    }

    auto *session = new ClientSession(socket, this);
    m_sessions.append(session);

    connect(session, &ClientSession::disconnected,
            this,    &PosServer::onClientDisconnected);
    connect(session, &ClientSession::buttonPressed,
            this,    &PosServer::onButtonPressed);
}

void PosServer::onClientDisconnected(ClientSession *session)
{
    m_sessions.removeOne(session);
    session->deleteLater();
    qInfo() << "[server] Active clients:" << m_sessions.size();
}

void PosServer::onButtonPressed(ClientSession *session)
{
    qInfo() << "[server] Button pressed acknowledged for client:" << session->id();
    // Future: broadcast to other clients, update order state, etc.
}

} // namespace vt
