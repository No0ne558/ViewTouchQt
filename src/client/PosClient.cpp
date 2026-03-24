// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/PosClient.cpp

#include "PosClient.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>


namespace vt {

PosClient::PosClient(QObject *parent)
    : QObject(parent)
{
    connect(&m_socket, &QTcpSocket::connected,    this, &PosClient::onConnected);
    connect(&m_socket, &QTcpSocket::disconnected,  this, &PosClient::onDisconnected);
    connect(&m_socket, &QTcpSocket::readyRead,     this, &PosClient::onReadyRead);

    m_reconnectTimer.setInterval(3000);
    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &PosClient::onReconnect);
}

void PosClient::connectToServer(const QString &host, quint16 port)
{
    m_host = host;
    m_port = port;
    qInfo() << "[client] Connecting to" << host << ":" << port;
    m_socket.connectToHost(host, port);
}

void PosClient::send(MsgType type, const QByteArray &payload)
{
    if (m_socket.state() == QAbstractSocket::ConnectedState) {
        m_socket.write(buildMessage(type, payload));
        m_socket.flush();
    }
}

// ── Slots ───────────────────────────────────────────────────────────────────

void PosClient::onConnected()
{
    qInfo() << "[client] Connected to server";
    m_reconnectTimer.stop();
    emit connected();
}

void PosClient::onDisconnected()
{
    qInfo() << "[client] Disconnected — will retry in 3 s";
    m_recvBuf.clear();
    m_reconnectTimer.start();
    emit disconnected();
}

void PosClient::onReadyRead()
{
    m_recvBuf.append(m_socket.readAll());
    processBuffer();
}

void PosClient::onReconnect()
{
    qInfo() << "[client] Reconnecting...";
    m_socket.connectToHost(m_host, m_port);
}

// ── Internal ────────────────────────────────────────────────────────────────

void PosClient::processBuffer()
{
    while (m_recvBuf.size() >= kHeaderSize) {
        Header hdr;
        if (!deserializeHeader(m_recvBuf, hdr)) {
            m_recvBuf.remove(0, 1);
            continue;
        }

        const int totalLen = kHeaderSize + hdr.payloadLen;
        if (m_recvBuf.size() < totalLen)
            return;

        QByteArray payload = m_recvBuf.mid(kHeaderSize, hdr.payloadLen);
        m_recvBuf.remove(0, totalLen);

        handleMessage(hdr, payload);
    }
}

void PosClient::handleMessage(const Header &hdr, const QByteArray &payload)
{
    switch (hdr.type) {
    case MsgType::Heartbeat:
        // Reply with ack immediately.
        send(MsgType::HeartbeatAck);
        break;

    case MsgType::ButtonAck:
        qDebug() << "[client] Button ack received";
        emit buttonAckReceived();
        break;

    case MsgType::LayoutSync:
        qInfo() << "[client] Layout sync received," << payload.size() << "bytes";
        emit layoutSyncReceived(payload);
        break;

    case MsgType::ValidatePinResponse: {
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(payload, &err);
        if (err.error != QJsonParseError::NoError) {
            qWarning() << "[client] Failed to parse ValidatePinResponse:" << err.errorString();
            break;
        }
        QJsonObject obj = doc.object();
        QString requestId = obj.value("requestId").toString();
        bool success = obj.value("success").toBool();
        QString username = obj.value("username").toString();
        QString role = obj.value("role").toString();
        QString message = obj.value("message").toString();
        emit validatePinResult(requestId, success, username, role, message);
        break;
    }

    default:
        qDebug() << "[client] Unknown message type:" << static_cast<int>(hdr.type);
        break;
    }
}

} // namespace vt
