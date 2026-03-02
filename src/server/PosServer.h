// SPDX-License-Identifier: GPL-3.0-or-later
// src/server/PosServer.h — TCP server that accepts terminal clients.

#ifndef VT_POS_SERVER_H
#define VT_POS_SERVER_H

#include "ClientSession.h"

#include <QList>
#include <QTcpServer>

namespace vt {

class PosServer : public QTcpServer {
    Q_OBJECT
public:
    explicit PosServer(QObject *parent = nullptr);

    /// Start listening on the given address/port. Returns true on success.
    bool startListening(const QHostAddress &address, quint16 port);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onClientDisconnected(ClientSession *session);
    void onButtonPressed(ClientSession *session);

private:
    QList<ClientSession *> m_sessions;
};

} // namespace vt

#endif // VT_POS_SERVER_H
