// SPDX-License-Identifier: GPL-3.0-or-later
// src/common/Protocol.cpp

#include "Protocol.h"
#include <QBuffer>

namespace vt {

QByteArray serializeHeader(const Header &hdr)
{
    QByteArray buf;
    buf.reserve(kHeaderSize);

    QBuffer device(&buf);
    device.open(QIODevice::WriteOnly);

    QDataStream out(&device);
    out.setByteOrder(QDataStream::BigEndian);
    out << hdr.magic
        << static_cast<std::uint16_t>(hdr.type)
        << hdr.payloadLen;

    return buf;
}

bool deserializeHeader(const QByteArray &data, Header &hdr)
{
    if (data.size() < kHeaderSize)
        return false;

    QDataStream in(data);
    in.setByteOrder(QDataStream::BigEndian);

    std::uint32_t magic{};
    std::uint16_t type{};
    std::uint16_t payloadLen{};

    in >> magic >> type >> payloadLen;

    if (magic != kProtocolMagic)
        return false;

    hdr.magic      = magic;
    hdr.type       = static_cast<MsgType>(type);
    hdr.payloadLen = payloadLen;
    return true;
}

QByteArray buildMessage(MsgType type, const QByteArray &payload)
{
    Header hdr;
    hdr.type       = type;
    hdr.payloadLen = static_cast<std::uint16_t>(payload.size());

    QByteArray msg = serializeHeader(hdr);
    if (!payload.isEmpty())
        msg.append(payload);
    return msg;
}

} // namespace vt
