/**
 * @file hlink_protocol.h
 * @brief H-Link v2.0 Packet Protocol
 * @description Custom lightweight protocol for tri-mode communication.
 *              Adds timestamp field for anti-replay, flags for priority/fragmentation.
 */

#ifndef HLINK_PROTOCOL_H
#define HLINK_PROTOCOL_H

#include "config.h"
#include <cstdint>
#include <cstring>

namespace hlink {

// ============================================================================
// PACKET TYPE DEFINITIONS
// ============================================================================
enum PacketType : uint8_t {
    TYPE_TEXT       = 0x01,  // Chat text message (UTF-8, max 240 bytes)
    TYPE_FILE_CHUNK = 0x02,  // File transfer chunk (index + binary)
    TYPE_ACK        = 0x03,  // Acknowledgment (SEQ number)
    TYPE_HEARTBEAT  = 0x04,  // LoRa periodic keepalive
    TYPE_GPS        = 0x05,  // GPS coordinates (lat + lon + timestamp)
    TYPE_BATTERY    = 0x06,  // Battery voltage + percent + runtime
    TYPE_VOICE      = 0x07,  // Voice audio frame (Codec2/Opus)
    TYPE_VOICE_REQ  = 0x08,  // Voice call request
    TYPE_VOICE_ACK  = 0x09,  // Voice call accepted/rejected
    TYPE_EMERGENCY  = 0x0A,  // Emergency broadcast (sent on ALL links)
    TYPE_LINK_STATUS= 0x0B,  // Link health report
    TYPE_FILE_META  = 0x0C,  // File metadata (name, size, chunks, hash)
    TYPE_ENCRYPTION = 0x0D,  // Encryption handshake / key confirmation
    TYPE_CMD        = 0x0E,  // Remote command (mode switch, alignment, etc.)
    TYPE_NACK       = 0x0F,  // Negative acknowledgment (request retransmit)
    TYPE_PING       = 0x10,  // Latency measurement ping
    TYPE_PONG       = 0x11,  // Latency measurement pong
};

// ============================================================================
// PACKET FLAGS
// ============================================================================
enum PacketFlags : uint8_t {
    FLAG_NONE       = 0x00,
    FLAG_ENCRYPTED  = 0x01,  // Payload is AES-256-GCM encrypted
    FLAG_PRIORITY   = 0x02,  // High priority (bypass normal queue)
    FLAG_FRAGMENT   = 0x04,  // Part of a fragmented payload
    FLAG_LAST_FRAG  = 0x08,  // Last fragment
    FLAG_COMPRESSED = 0x10,  // Payload is compressed
    FLAG_BROADCAST  = 0x20,  // Send on all available links
};

// ============================================================================
// PACKET STRUCTURE
// ============================================================================
#pragma pack(push, 1)
struct PacketHeader {
    uint8_t  sync[2];       // 0xAA 0x55
    uint8_t  type;          // PacketType
    uint16_t seq;           // Sequence number (big-endian)
    uint32_t timestamp;     // Unix timestamp (seconds) for anti-replay
    uint8_t  flags;         // PacketFlags
    uint16_t length;        // Payload length (big-endian), max 240
};
#pragma pack(pop)

#define HLINK_HEADER_SIZE     (sizeof(PacketHeader))  // 10 bytes
#define HLINK_MAX_TOTAL_SIZE  (HLINK_HEADER_SIZE + HLINK_MAX_PAYLOAD + 1) // 251 bytes

// ============================================================================
// PACKET CLASS
// ============================================================================
class Packet {
public:
    PacketHeader header;
    uint8_t payload[HLINK_MAX_PAYLOAD];
    uint8_t crc;            // CRC-8 over payload only

    Packet();
    Packet(PacketType type, uint16_t seq, const uint8_t* data, uint16_t len, uint8_t flags = FLAG_NONE);

    // Serialization
    int serialize(uint8_t* buffer, size_t bufferSize) const;

    // Deserialization (returns bytes consumed, 0 on error)
    int deserialize(const uint8_t* buffer, size_t bufferSize);

    // Validation
    bool isValid() const;
    bool verifyCRC() const;
    void computeCRC();

    // Utility
    static bool isSync(const uint8_t* buffer);
    static const char* typeToString(PacketType type);
    static uint16_t nextSequence();

    void setPayload(const uint8_t* data, uint16_t len);
    void clear();

    size_t totalSize() const { return HLINK_HEADER_SIZE + header.length + 1; }
};

// ============================================================================
// CRC-8 IMPLEMENTATION (ITU polynomial 0x07)
// ============================================================================
class CRC8 {
public:
    static uint8_t compute(const uint8_t* data, size_t len);
    static uint8_t compute(const Packet& pkt);
    static void generateTable();
    static bool tableInitialized;
private:
    static uint8_t crcTable[256];
};

// ============================================================================
// PACKET QUEUE (for ARQ retransmission)
// ============================================================================
struct QueuedPacket {
    Packet packet;
    uint32_t sendTime;
    uint8_t retryCount;
    bool pending;

    void reset() {
        pending = false;
        retryCount = 0;
        sendTime = 0;
        packet.clear();
    }
};

// ============================================================================
// ARQ MANAGER (Stop-and-Wait with timeout)
// ============================================================================
class ARQManager {
public:
    ARQManager(uint16_t timeoutMs = ARQ_TIMEOUT_WIFI_MS, uint8_t maxRetries = ARQ_MAX_RETRIES);

    bool sendPacket(const Packet& pkt);
    bool onAckReceived(uint16_t seq);
    bool onNackReceived(uint16_t seq);
    void update();  // Call every tick to check timeouts
    bool hasPending() const { return pending; }
    uint16_t getPendingSeq() const { return pendingSeq; }
    void reset();

private:
    QueuedPacket queue[4];      // Small window for efficiency
    static constexpr size_t QUEUE_SIZE = 4;
    uint16_t timeoutMs;
    uint8_t maxRetries;
    bool pending;
    uint16_t pendingSeq;
    uint32_t lastSendTime;
    uint8_t currentSlot;
};

} // namespace hlink

#endif // HLINK_PROTOCOL_H
