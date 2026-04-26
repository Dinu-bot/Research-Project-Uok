"""
Threaded UDP Client for ESP32 Communication
"""

import socket
import threading
import queue
import time
from typing import Callable, Optional
from PyQt6.QtCore import QObject, pyqtSignal

from hlink_protocol import HLinkPacket, PacketType


class UDPClient(QObject):
    """Threaded UDP client with packet queue."""

    # Signals for Qt thread safety
    packet_received = pyqtSignal(object)  # HLinkPacket
    connected = pyqtSignal(bool)
    error = pyqtSignal(str)

    def __init__(self, local_port: int = 4210, remote_port: int = 4210):
        super().__init__()
        self.local_port = local_port
        self.remote_port = remote_port
        self.remote_addr: Optional[tuple] = None

        self._socket: Optional[socket.socket] = None
        self._running = False
        self._thread: Optional[threading.Thread] = None
        self._tx_queue: queue.Queue = queue.Queue(maxsize=256)
        self._tx_thread: Optional[threading.Thread] = None

        self._connected = False
        self._last_rx_time = 0.0
        self._stats = {"tx": 0, "rx": 0, "tx_bytes": 0, "rx_bytes": 0, "errors": 0}

    def start(self, remote_ip: str) -> bool:
        """Start UDP client."""
        try:
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self._socket.bind(("0.0.0.0", self.local_port))
            self._socket.settimeout(1.0)

            self.remote_addr = (remote_ip, self.remote_port)
            self._running = True

            # Start RX thread
            self._thread = threading.Thread(target=self._rx_loop, daemon=True)
            self._thread.start()

            # Start TX thread
            self._tx_thread = threading.Thread(target=self._tx_loop, daemon=True)
            self._tx_thread.start()

            self._connected = True
            self.connected.emit(True)
            print(f"[UDP] Connected to {remote_ip}:{self.remote_port}")
            return True

        except Exception as e:
            self.error.emit(str(e))
            return False

    def stop(self):
        """Stop UDP client."""
        self._running = False
        self._connected = False

        if self._thread:
            self._thread.join(timeout=2.0)
        if self._tx_thread:
            self._tx_thread.join(timeout=2.0)
        if self._socket:
            self._socket.close()
            self._socket = None

        self.connected.emit(False)
        print("[UDP] Disconnected")

    def send(self, packet: HLinkPacket) -> bool:
        """Queue packet for transmission."""
        try:
            self._tx_queue.put(packet, block=False)
            return True
        except queue.Full:
            self._stats["errors"] += 1
            return False

    def is_connected(self) -> bool:
        return self._connected and (time.time() - self._last_rx_time < 5.0)

    def get_stats(self) -> dict:
        return self._stats.copy()

    def _rx_loop(self):
        """Receive loop running in separate thread."""
        buffer = bytearray(2048)

        while self._running:
            try:
                self._socket.settimeout(1.0)
                nbytes, addr = self._socket.recvfrom_into(buffer)

                if nbytes > 0:
                    self._last_rx_time = time.time()
                    self._stats["rx"] += 1
                    self._stats["rx_bytes"] += nbytes

                    # Parse packet
                    data = bytes(buffer[:nbytes])
                    pkt = HLinkPacket()
                    if pkt.deserialize(data):
                        self.packet_received.emit(pkt)

            except socket.timeout:
                continue
            except Exception as e:
                if self._running:
                    self._stats["errors"] += 1
                    self.error.emit(str(e))

    def _tx_loop(self):
        """Transmit loop running in separate thread."""
        while self._running:
            try:
                packet = self._tx_queue.get(timeout=0.1)
                if self._socket and self.remote_addr:
                    data = packet.serialize()
                    self._socket.sendto(data, self.remote_addr)
                    self._stats["tx"] += 1
                    self._stats["tx_bytes"] += len(data)
            except queue.Empty:
                continue
            except Exception as e:
                if self._running:
                    self._stats["errors"] += 1
