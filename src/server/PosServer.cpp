// SPDX-License-Identifier: GPL-3.0-or-later
// src/server/PosServer.cpp

#include "PosServer.h"

#include <QDebug>
#include <QTcpSocket>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

namespace vt {

PosServer::PosServer(QObject *parent)
    : QTcpServer(parent)
{
    // Default to a minimal Login-only layout so standalone servers provide
    // a predictable starting layout for newly connected clients.
    m_currentLayout = QStringLiteral("{\"version\":1,\"pages\":[{\"name\":\"Login\",\"systemPage\":true,\"elements\":[]}]}").toUtf8();

    // Hardcode a simple in-memory user store: Super User with PIN 13524
    UserRecord superUser;
    superUser.username = QStringLiteral("Super User");
    superUser.pin = QStringLiteral("13524");
    superUser.role = QStringLiteral("super");
    m_usersByPin.insert(superUser.pin, superUser);
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
    // Persist layout to system data directory so it survives restarts.
    QString systemDatPath = QStringLiteral("/opt/viewtouch/dat");
    QDir datDir(systemDatPath);
    if (!datDir.exists()) {
        if (!QDir().mkpath(systemDatPath)) {
            qWarning() << "[server] Failed to create layout directory:" << systemDatPath;
        }
    }
    QString layoutPath = datDir.filePath(QStringLiteral("layout.json"));
    QFile f(layoutPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(m_currentLayout);
        f.close();
        qInfo() << "[server] Saved layout to" << layoutPath;
    } else {
        qWarning() << "[server] Failed to save layout to" << layoutPath << ":" << f.errorString();
    }

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

void PosServer::validatePin(ClientSession *session, const QString &requestId, const QString &pin)
{
    QJsonObject resp;
    resp[QStringLiteral("requestId")] = requestId;

    if (m_usersByPin.contains(pin)) {
        const UserRecord &r = m_usersByPin.value(pin);
        resp[QStringLiteral("success")] = true;
        resp[QStringLiteral("username")] = r.username;
        resp[QStringLiteral("role")] = r.role;
        resp[QStringLiteral("message")] = QString();
        // Optionally persist session auth state here (future)
        qInfo() << "[server] PIN validated for user:" << r.username << "client:" << session->id();
    } else {
        resp[QStringLiteral("success")] = false;
        resp[QStringLiteral("username")] = QString();
        resp[QStringLiteral("role")] = QString();
        resp[QStringLiteral("message")] = QStringLiteral("Invalid PIN");
        qInfo() << "[server] Invalid PIN attempt from client:" << session->id();
    }

    QJsonDocument doc(resp);
    session->send(MsgType::ValidatePinResponse, doc.toJson(QJsonDocument::Compact));
}

} // namespace vt
