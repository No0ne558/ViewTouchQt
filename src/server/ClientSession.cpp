// SPDX-License-Identifier: GPL-3.0-or-later
// src/server/ClientSession.cpp

#include "ClientSession.h"

#include <QDebug>

namespace vt {

ClientSession::ClientSession(QTcpSocket *socket, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
{
    m_socket->setParent(this);

    connect(m_socket, &QTcpSocket::readyRead,    this, &ClientSession::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected,  this, &ClientSession::onDisconnected);

    // Heartbeat: ping every 5 s
    m_heartbeatTimer.setInterval(5000);
    connect(&m_heartbeatTimer, &QTimer::timeout, this, &ClientSession::onHeartbeatTick);
    m_heartbeatTimer.start();

    // Timeout: if no pong within 15 s → drop
    m_timeoutTimer.setInterval(15000);
    m_timeoutTimer.setSingleShot(true);
    connect(&m_timeoutTimer, &QTimer::timeout, this, &ClientSession::onHeartbeatTimeout);

    qInfo() << "[server] Client connected:" << m_socket->peerAddress().toString()
            << "port" << m_socket->peerPort();
}

ClientSession::~ClientSession()
{
    qInfo() << "[server] ClientSession destroyed, id:" << id();
}

void ClientSession::send(MsgType type, const QByteArray &payload)
{
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(buildMessage(type, payload));
        m_socket->flush();
    }
}

// ── Slots ───────────────────────────────────────────────────────────────────

void ClientSession::onReadyRead()
{
    m_recvBuf.append(m_socket->readAll());
    processBuffer();
}

void ClientSession::onDisconnected()
{
    qInfo() << "[server] Client disconnected:" << id();
    m_heartbeatTimer.stop();
    m_timeoutTimer.stop();
    emit disconnected(this);
}

void ClientSession::onHeartbeatTick()
{
    send(MsgType::Heartbeat);
    if (!m_awaitingPong) {
        m_awaitingPong = true;
        m_timeoutTimer.start();
    }
}

void ClientSession::onHeartbeatTimeout()
{
    qWarning() << "[server] Heartbeat timeout for client:" << id();
    m_socket->disconnectFromHost();
}

// ── Internal ────────────────────────────────────────────────────────────────

void ClientSession::processBuffer()
{
    while (m_recvBuf.size() >= kHeaderSize) {
        Header hdr;
        if (!deserializeHeader(m_recvBuf, hdr)) {
            // Bad magic — discard one byte and retry.
            m_recvBuf.remove(0, 1);
            continue;
        }

        const int totalLen = kHeaderSize + hdr.payloadLen;
        if (m_recvBuf.size() < totalLen)
            return;  // wait for more data

        QByteArray payload = m_recvBuf.mid(kHeaderSize, hdr.payloadLen);
        m_recvBuf.remove(0, totalLen);

        handleMessage(hdr, payload);
    }
}

void ClientSession::handleMessage(const Header &hdr, const QByteArray & /*payload*/)
{
    switch (hdr.type) {
    case MsgType::HeartbeatAck:
        m_awaitingPong = false;
        m_timeoutTimer.stop();
        break;

    case MsgType::ButtonPress:
        qInfo() << "[server] Button press from client:" << id();
        send(MsgType::ButtonAck);
        emit buttonPressed(this);
        break;

    default:
        qDebug() << "[server] Unknown message type:" << static_cast<int>(hdr.type);
        break;
    }
}

} // namespace vt
