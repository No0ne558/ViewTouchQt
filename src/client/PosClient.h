// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/PosClient.h — TCP client wrapper for communicating with vt_server.

#ifndef VT_POS_CLIENT_H
#define VT_POS_CLIENT_H

#include "Protocol.h"

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

namespace vt {

class PosClient : public QObject {
    Q_OBJECT
public:
    explicit PosClient(QObject *parent = nullptr);

    /// Connect to the server.  Automatically retries every 3 s on failure.
    void connectToServer(const QString &host, quint16 port);

    /// Send a message to the server.
    void send(MsgType type, const QByteArray &payload = {});

signals:
    void connected();
    void disconnected();
    void buttonAckReceived();
    void layoutSyncReceived(const QByteArray &layoutJson);
    void validatePinResult(const QString &requestId, bool success, const QString &username, const QString &role, const QString &message);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onReconnect();

private:
    void processBuffer();
    void handleMessage(const Header &hdr, const QByteArray &payload);

    QTcpSocket m_socket;
    QByteArray m_recvBuf;
    QTimer     m_reconnectTimer;

    QString    m_host;
    quint16    m_port = kDefaultPort;
};

} // namespace vt

#endif // VT_POS_CLIENT_H
