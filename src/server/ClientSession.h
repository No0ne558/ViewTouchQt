// SPDX-License-Identifier: GPL-3.0-or-later
// src/server/ClientSession.h — Manages a single client connection.

#ifndef VT_CLIENT_SESSION_H
#define VT_CLIENT_SESSION_H

#include "Protocol.h"

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

namespace vt {

class ClientSession : public QObject {
    Q_OBJECT
public:
    explicit ClientSession(QTcpSocket *socket, QObject *parent = nullptr);
    ~ClientSession() override;

    /// Unique id for logging.
    quintptr id() const { return reinterpret_cast<quintptr>(m_socket); }

    /// Send a raw message to this client.
    void send(MsgType type, const QByteArray &payload = {});

signals:
    /// Emitted when the connection is lost or timed-out.
    void disconnected(ClientSession *session);

    /// Emitted when the client sends a button press.
    void buttonPressed(ClientSession *session);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onHeartbeatTick();
    void onHeartbeatTimeout();

private:
    void processBuffer();
    void handleMessage(const Header &hdr, const QByteArray &payload);

    QTcpSocket *m_socket       = nullptr;

    QByteArray  m_recvBuf;           // accumulation buffer

    QTimer      m_heartbeatTimer;    // sends heartbeat every 5 s
    QTimer      m_timeoutTimer;      // disconnects if no response in 15 s
    bool        m_awaitingPong = false;
};

} // namespace vt

#endif // VT_CLIENT_SESSION_H
