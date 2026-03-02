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

void PosServer::setCurrentLayout(const QByteArray &layoutJson)
{
    m_currentLayout = layoutJson;
    qInfo() << "[server] Layout updated," << layoutJson.size() << "bytes — broadcasting to"
            << m_sessions.size() << "client(s)";
    broadcastToAll(MsgType::LayoutSync, m_currentLayout);
}

void PosServer::broadcastToAll(MsgType type, const QByteArray &payload)
{
    for (ClientSession *s : m_sessions)
        s->send(type, payload);
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

    // Push the current layout to the newly connected client.
    if (!m_currentLayout.isEmpty()) {
        qInfo() << "[server] Sending stored layout to new client:" << session->id();
        session->send(MsgType::LayoutSync, m_currentLayout);
    }
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
