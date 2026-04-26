/**
 * @file hlink_protocol.cpp
 * @brief H-Link v2.0 Protocol Implementation
 */

#include "hlink_protocol.h"
#include <Arduino.h>

namespace hlink {

// ============================================================================
// CRC-8 TABLE
// ============================================================================
uint8_t CRC8::crcTable[256];
bool CRC8::tableInitialized = false;

void CRC8::generateTable() {
    if (tableInitialized) return;
    for (int i = 0; i < 256; i++) {
        uint8_t crc = i;
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x80) ? ((crc << 1) ^ CRC8_POLYNOMIAL) : (crc << 1);
        }
        crcTable[i] = crc;
    }
    tableInitialized = true;
}

uint8_t CRC8::compute(const uint8_t* data, size_t len) {
    if (!tableInitialized) generateTable();
    uint8_t crc = CRC8_INITIAL;
    for (size_t i = 0; i < len; i++) {
        crc = crcTable[crc ^ data[i]];
    }
    return crc;
}

uint8_t CRC8::compute(const Packet& pkt) {
    return compute(pkt.payload, pkt.header.length);
}

// ============================================================================
// PACKET METHODS
// ============================================================================
static uint16_t globalSequence = 0;

Packet::Packet() {
    clear();
}

Packet::Packet(PacketType type, uint16_t seq, const uint8_t* data, uint16_t len, uint8_t flags) {
    clear();
    header.sync[0] = HLINK_SYNC_BYTE_1;
    header.sync[1] = HLINK_SYNC_BYTE_2;
    header.type = static_cast<uint8_t>(type);
    header.seq = seq;
    header.timestamp = 0;  // Set by sender
    header.flags = flags;
    header.length = (len > HLINK_MAX_PAYLOAD) ? HLINK_MAX_PAYLOAD : len;
    if (data && header.length > 0) {
        memcpy(payload, data, header.length);
    }
    computeCRC();
}

void Packet::clear() {
    memset(&header, 0, sizeof(header));
    memset(payload, 0, sizeof(payload));
    crc = 0;
}

void Packet::setPayload(const uint8_t* data, uint16_t len) {
    header.length = (len > HLINK_MAX_PAYLOAD) ? HLINK_MAX_PAYLOAD : len;
    if (data && header.length > 0) {
        memcpy(payload, data, header.length);
    }
}

void Packet::computeCRC() {
    crc = CRC8::compute(payload, header.length);
}

bool Packet::verifyCRC() const {
    if (header.length > HLINK_MAX_PAYLOAD) return false;
    uint8_t computed = CRC8::compute(payload, header.length);
    return computed == crc;
}

bool Packet::isValid() const {
    return (header.sync[0] == HLINK_SYNC_BYTE_1) && 
           (header.sync[1] == HLINK_SYNC_BYTE_2) &&
           (header.length <= HLINK_MAX_PAYLOAD) &&
           verifyCRC();
}

bool Packet::isSync(const uint8_t* buffer) {
    return (buffer[0] == HLINK_SYNC_BYTE_1) && (buffer[1] == HLINK_SYNC_BYTE_2);
}

int Packet::serialize(uint8_t* buffer, size_t bufferSize) const {
    size_t total = totalSize();
    if (bufferSize < total) return -1;

    size_t idx = 0;
    buffer[idx++] = header.sync[0];
    buffer[idx++] = header.sync[1];
    buffer[idx++] = header.type;
    buffer[idx++] = (header.seq >> 8) & 0xFF;
    buffer[idx++] = header.seq & 0xFF;
    buffer[idx++] = (header.timestamp >> 24) & 0xFF;
    buffer[idx++] = (header.timestamp >> 16) & 0xFF;
    buffer[idx++] = (header.timestamp >> 8) & 0xFF;
    buffer[idx++] = header.timestamp & 0xFF;
    buffer[idx++] = header.flags;
    buffer[idx++] = (header.length >> 8) & 0xFF;
    buffer[idx++] = header.length & 0xFF;

    if (header.length > 0) {
        memcpy(&buffer[idx], payload, header.length);
        idx += header.length;
    }
    buffer[idx++] = crc;

    return static_cast<int>(idx);
}

int Packet::deserialize(const uint8_t* buffer, size_t bufferSize) {
    if (bufferSize < HLINK_HEADER_SIZE + 1) return 0;
    if (!isSync(buffer)) return 0;

    size_t idx = 0;
    header.sync[0] = buffer[idx++];
    header.sync[1] = buffer[idx++];
    header.type = buffer[idx++];
    header.seq = (static_cast<uint16_t>(buffer[idx]) << 8) | buffer[idx+1]; idx += 2;
    header.timestamp = (static_cast<uint32_t>(buffer[idx]) << 24) |
                       (static_cast<uint32_t>(buffer[idx+1]) << 16) |
                       (static_cast<uint32_t>(buffer[idx+2]) << 8) |
                       static_cast<uint32_t>(buffer[idx+3]); idx += 4;
    header.flags = buffer[idx++];
    header.length = (static_cast<uint16_t>(buffer[idx]) << 8) | buffer[idx+1]; idx += 2;

    if (header.length > HLINK_MAX_PAYLOAD) return 0;
    if (bufferSize < idx + header.length + 1) return 0;  // Need more data

    if (header.length > 0) {
        memcpy(payload, &buffer[idx], header.length);
        idx += header.length;
    }
    crc = buffer[idx++];

    return isValid() ? static_cast<int>(idx) : 0;
}

const char* Packet::typeToString(PacketType type) {
    switch (type) {
        case TYPE_TEXT: return "TEXT";
        case TYPE_FILE_CHUNK: return "FILE_CHUNK";
        case TYPE_ACK: return "ACK";
        case TYPE_HEARTBEAT: return "HEARTBEAT";
        case TYPE_GPS: return "GPS";
        case TYPE_BATTERY: return "BATTERY";
        case TYPE_VOICE: return "VOICE";
        case TYPE_VOICE_REQ: return "VOICE_REQ";
        case TYPE_VOICE_ACK: return "VOICE_ACK";
        case TYPE_EMERGENCY: return "EMERGENCY";
        case TYPE_LINK_STATUS: return "LINK_STATUS";
        case TYPE_FILE_META: return "FILE_META";
        case TYPE_ENCRYPTION: return "ENCRYPTION";
        case TYPE_CMD: return "CMD";
        case TYPE_NACK: return "NACK";
        case TYPE_PING: return "PING";
        case TYPE_PONG: return "PONG";
        default: return "UNKNOWN";
    }
}

uint16_t Packet::nextSequence() {
    globalSequence++;
    if (globalSequence > HLINK_SEQ_MAX) globalSequence = 1;
    return globalSequence;
}

// ============================================================================
// ARQ MANAGER
// ============================================================================
ARQManager::ARQManager(uint16_t to, uint8_t retries) 
    : timeoutMs(to), maxRetries(retries), pending(false), pendingSeq(0), 
      lastSendTime(0), currentSlot(0) {
    for (auto& q : queue) q.reset();
}

bool ARQManager::sendPacket(const Packet& pkt) {
    if (pending) return false;  // Stop-and-wait: only one pending at a time

    // Find free slot
    size_t slot = currentSlot;
    queue[slot].packet = pkt;
    queue[slot].sendTime = millis();
    queue[slot].retryCount = 0;
    queue[slot].pending = true;

    pending = true;
    pendingSeq = pkt.header.seq;
    lastSendTime = millis();
    currentSlot = (currentSlot + 1) % QUEUE_SIZE;

    return true;
}

bool ARQManager::onAckReceived(uint16_t seq) {
    if (!pending || seq != pendingSeq) return false;

    for (auto& q : queue) {
        if (q.pending && q.packet.header.seq == seq) {
            q.reset();
            pending = false;
            pendingSeq = 0;
            return true;
        }
    }
    return false;
}

bool ARQManager::onNackReceived(uint16_t seq) {
    for (auto& q : queue) {
        if (q.pending && q.packet.header.seq == seq) {
            q.retryCount = maxRetries;  // Force immediate retransmit on next update
            return true;
        }
    }
    return false;
}

void ARQManager::update() {
    if (!pending) return;

    uint32_t now = millis();
    for (auto& q : queue) {
        if (!q.pending) continue;

        if (now - q.sendTime >= timeoutMs) {
            q.retryCount++;
            if (q.retryCount > maxRetries) {
                // Packet failed after max retries
                q.reset();
                pending = false;
                pendingSeq = 0;
                // TODO: Notify link manager of failure
            } else {
                // Retransmit
                q.sendTime = now;
                lastSendTime = now;
                // The actual retransmit is handled by caller checking hasPending()
            }
        }
    }
}

void ARQManager::reset() {
    for (auto& q : queue) q.reset();
    pending = false;
    pendingSeq = 0;
}

} // namespace hlink
