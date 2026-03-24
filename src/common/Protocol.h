// SPDX-License-Identifier: GPL-3.0-or-later
// src/common/Protocol.h — Binary protocol shared between vt_server and vt_client.

#ifndef VT_PROTOCOL_H
#define VT_PROTOCOL_H

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <cstdint>

namespace vt {

// ── Magic number: ASCII "VTQT" ──────────────────────────────────────────────
constexpr std::uint32_t kProtocolMagic = 0x56545154;

// ── Default listen port ─────────────────────────────────────────────────────
constexpr std::uint16_t kDefaultPort = 12000;

// ── Message types ───────────────────────────────────────────────────────────
enum class MsgType : std::uint16_t {
    Heartbeat    = 0x0001,
    HeartbeatAck = 0x0002,
    ButtonPress  = 0x0010,
    ButtonAck    = 0x0011,
    LayoutSync   = 0x0020,   // server → client: full layout JSON payload
    // Authentication messages
    ValidatePinRequest  = 0x0030, // client → server: JSON {requestId,page,elementId,pin}
    ValidatePinResponse = 0x0031, // server → client: JSON {requestId,success,username,role,message}
};

// ── Fixed header (10 bytes) ─────────────────────────────────────────────────
//  [ magic 4B ][ type 2B ][ payload_len 4B ]
struct Header {
    std::uint32_t magic      = kProtocolMagic;
    MsgType       type       = MsgType::Heartbeat;
    std::uint32_t payloadLen = 0;
};

// ── Convenience: fixed header size ──────────────────────────────────────────
constexpr int kHeaderSize = 10;  // 4 + 2 + 4

// ── Serialize a header into a byte array ────────────────────────────────────
QByteArray serializeHeader(const Header &hdr);

// ── Deserialize a header from raw bytes (must be >= kHeaderSize).
//    Returns false if magic doesn't match. ───────────────────────────────────
bool deserializeHeader(const QByteArray &data, Header &hdr);

// ── Build a complete message (header + optional payload) ────────────────────
QByteArray buildMessage(MsgType type, const QByteArray &payload = {});

} // namespace vt

#endif // VT_PROTOCOL_H
