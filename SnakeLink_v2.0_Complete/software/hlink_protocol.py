"""
H-Link Protocol v2.0 — Python Implementation
"""

import struct
import time
from enum import IntEnum
from typing import Optional, Tuple
from dataclasses import dataclass


class PacketType(IntEnum):
    TEXT = 0x01
    FILE_CHUNK = 0x02
    ACK = 0x03
    HEARTBEAT = 0x04
    GPS = 0x05
    BATTERY = 0x06
    VOICE = 0x07
    VOICE_REQ = 0x08
    VOICE_ACK = 0x09
    EMERGENCY = 0x0A
    LINK_STATUS = 0x0B
    FILE_META = 0x0C
    ENCRYPTION = 0x0D
    CMD = 0x0E
    NACK = 0x0F
    PING = 0x10
    PONG = 0x11


class PacketFlags(IntEnum):
    NONE = 0x00
    ENCRYPTED = 0x01
    PRIORITY = 0x02
    FRAGMENT = 0x04
    LAST_FRAG = 0x08
    COMPRESSED = 0x10
    BROADCAST = 0x20


def crc8_itu(data: bytes) -> int:
    """CRC-8 ITU polynomial 0x07."""
    crc = 0x00
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = ((crc << 1) ^ 0x07) if (crc & 0x80) else (crc << 1)
    return crc & 0xFF


@dataclass
class HLinkPacket:
    """H-Link packet structure."""
    sync: bytes = b"\xAA\x55"
    type: int = 0
    seq: int = 0
    timestamp: int = 0
    flags: int = 0
    length: int = 0
    payload: bytes = b""
    crc: int = 0

    _seq_counter = 0

    @classmethod
    def next_sequence(cls) -> int:
        cls._seq_counter = (cls._seq_counter + 1) & 0xFFFF
        return cls._seq_counter

    def compute_crc(self) -> int:
        return crc8_itu(self.payload)

    def is_valid(self) -> bool:
        if self.sync != b"\xAA\x55":
            return False
        if self.length > 240:
            return False
        return self.crc == self.compute_crc()

    def serialize(self) -> bytes:
        """Serialize packet to bytes."""
        self.crc = self.compute_crc()
        header = struct.pack(">BBHIBH",
            self.sync[0], self.sync[1],
            self.type & 0xFF,
            self.seq & 0xFFFF,
            self.timestamp & 0xFFFFFFFF,
            self.flags & 0xFF,
            self.length & 0xFFFF
        )
        return header + self.payload + bytes([self.crc])

    def deserialize(self, data: bytes) -> bool:
        """Deserialize bytes to packet."""
        if len(data) < 12:  # Minimum header size
            return False

        if data[0:2] != b"\xAA\x55":
            return False

        self.sync = data[0:2]
        self.type = data[2]
        self.seq = struct.unpack(">H", data[3:5])[0]
        self.timestamp = struct.unpack(">I", data[5:9])[0]
        self.flags = data[9]
        self.length = struct.unpack(">H", data[10:12])[0]

        if self.length > 240:
            return False

        if len(data) < 12 + self.length + 1:
            return False

        self.payload = data[12:12 + self.length]
        self.crc = data[12 + self.length]

        return self.is_valid()

    @classmethod
    def create(cls, ptype: PacketType, payload: bytes = b"", 
               flags: int = PacketFlags.NONE) -> "HLinkPacket":
        """Factory method to create a packet."""
        pkt = cls()
        pkt.type = ptype
        pkt.seq = cls.next_sequence()
        pkt.timestamp = int(time.time())
        pkt.flags = flags
        pkt.length = min(len(payload), 240)
        pkt.payload = payload[:pkt.length]
        pkt.crc = pkt.compute_crc()
        return pkt

    def type_name(self) -> str:
        try:
            return PacketType(self.type).name
        except:
            return f"UNKNOWN(0x{self.type:02X})"
