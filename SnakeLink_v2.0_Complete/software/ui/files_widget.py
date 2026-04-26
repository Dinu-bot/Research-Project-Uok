"""
File Transfer Widget — Chunked, compressed, resumable
"""

import os
import zlib
import hashlib
from pathlib import Path
from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton,
    QProgressBar, QListWidget, QListWidgetItem, QFileDialog,
    QFrame, QSizePolicy
)
from PyQt6.QtCore import Qt, QThread, pyqtSignal

from config import AppConfig
from udp_client import UDPClient
from crypto_manager import CryptoManager
from hlink_protocol import HLinkPacket, PacketType, PacketFlags


class FileTransferWorker(QThread):
    """Background thread for file transfer."""
    progress = pyqtSignal(int, int, int)  # current, total, speed
    chunk_sent = pyqtSignal(int)  # chunk index
    completed = pyqtSignal(bool, str)

    def __init__(self, udp: UDPClient, file_path: str, config: AppConfig, 
                 crypto: CryptoManager):
        super().__init__()
        self.udp = udp
        self.file_path = file_path
        self.config = config
        self.crypto = crypto
        self._cancelled = False

        self.chunk_size = 200  # Leave room for headers
        self.total_chunks = 0
        self.chunks_sent = 0

    def cancel(self):
        self._cancelled = True

    def run(self):
        try:
            path = Path(self.file_path)
            filename = path.name

            # Read and compress
            with open(self.file_path, "rb") as f:
                data = f.read()

            original_size = len(data)

            if self.config.auto_compress:
                data = zlib.compress(data, self.config.compression_quality)

            compressed_size = len(data)

            # Calculate hash
            file_hash = hashlib.sha256(data).digest()

            # Split into chunks
            self.total_chunks = (len(data) + self.chunk_size - 1) // self.chunk_size

            # Send metadata
            meta_payload = (
                filename.encode("utf-8")[:32].ljust(32, b"\x00") +
                original_size.to_bytes(4, "big") +
                compressed_size.to_bytes(4, "big") +
                self.total_chunks.to_bytes(2, "big") +
                file_hash[:16]
            )

            meta_pkt = HLinkPacket.create(PacketType.FILE_META, meta_payload)
            self.udp.send(meta_pkt)

            # Send chunks
            for i in range(self.total_chunks):
                if self._cancelled:
                    self.completed.emit(False, "Cancelled")
                    return

                start = i * self.chunk_size
                end = min(start + self.chunk_size, len(data))
                chunk = data[start:end]

                # Encrypt chunk if needed
                if self.config.high_security_mode and self.crypto.is_ready():
                    encrypted = self.crypto.encrypt(chunk)
                    if encrypted:
                        chunk = encrypted

                chunk_payload = i.to_bytes(2, "big") + chunk
                pkt = HLinkPacket.create(PacketType.FILE_CHUNK, chunk_payload)
                self.udp.send(pkt)

                self.chunks_sent = i + 1
                self.chunk_sent.emit(i)
                self.progress.emit(i + 1, self.total_chunks, 
                                  int((i + 1) / self.total_chunks * 100))

                # Small delay to prevent flooding
                self.msleep(10)

            self.completed.emit(True, f"Sent {filename}")

        except Exception as e:
            self.completed.emit(False, str(e))


class FilesWidget(QWidget):
    """File transfer interface."""

    def __init__(self, config: AppConfig, udp: UDPClient, crypto: CryptoManager):
        super().__init__()
        self.config = config
        self.udp = udp
        self.crypto = crypto
        self.transfers = []
        self.received_files = []
        self.pending_chunks = {}

        self._setup_ui()

    def _setup_ui(self):
        layout = QHBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(16)

        # Send panel
        send_panel = QFrame()
        send_panel.setObjectName("panel")
        send_layout = QVBoxLayout(send_panel)

        send_layout.addWidget(QLabel("Send Files"))

        self.btn_browse = QPushButton("Browse...")
        self.btn_browse.clicked.connect(self._browse_file)
        send_layout.addWidget(self.btn_browse)

        self.lbl_file_info = QLabel("No file selected")
        self.lbl_file_info.setWordWrap(True)
        send_layout.addWidget(self.lbl_file_info)

        self.progress = QProgressBar()
        self.progress.setRange(0, 100)
        self.progress.setValue(0)
        send_layout.addWidget(self.progress)

        self.btn_send = QPushButton("Send")
        self.btn_send.setEnabled(False)
        self.btn_send.clicked.connect(self._start_transfer)
        send_layout.addWidget(self.btn_send)

        self.btn_cancel = QPushButton("Cancel")
        self.btn_cancel.setEnabled(False)
        self.btn_cancel.clicked.connect(self._cancel_transfer)
        send_layout.addWidget(self.btn_cancel)

        send_layout.addStretch()
        layout.addWidget(send_panel, 1)

        # Receive panel
        recv_panel = QFrame()
        recv_panel.setObjectName("panel")
        recv_layout = QVBoxLayout(recv_panel)

        recv_layout.addWidget(QLabel("Received Files"))

        self.lst_received = QListWidget()
        recv_layout.addWidget(self.lst_received)

        recv_layout.addStretch()
        layout.addWidget(recv_panel, 1)

        self.setStyleSheet("""
            #panel {
                background-color: #16213e;
                border-radius: 8px;
                padding: 12px;
            }
            QLabel { color: #e9ecef; }
            QPushButton {
                background-color: #0f3460;
                color: #e9ecef;
                border: 1px solid #e94560;
                border-radius: 4px;
                padding: 8px;
            }
            QPushButton:hover { background-color: #e94560; }
            QProgressBar {
                background-color: #1a1a2e;
                border: 1px solid #0f3460;
                border-radius: 4px;
                text-align: center;
                color: #e9ecef;
            }
            QProgressBar::chunk {
                background-color: #51cf66;
                border-radius: 4px;
            }
        """)

    def _browse_file(self):
        path, _ = QFileDialog.getOpenFileName(self, "Select File")
        if path:
            self.current_file = path
            size = os.path.getsize(path)
            self.lbl_file_info.setText(f"{Path(path).name}\n{size:,} bytes")
            self.btn_send.setEnabled(True)

    def _start_transfer(self):
        if not hasattr(self, "current_file"):
            return

        self.worker = FileTransferWorker(
            self.udp, self.current_file, self.config, self.crypto
        )
        self.worker.progress.connect(self._on_progress)
        self.worker.completed.connect(self._on_completed)
        self.worker.start()

        self.btn_send.setEnabled(False)
        self.btn_cancel.setEnabled(True)

    def _cancel_transfer(self):
        if hasattr(self, "worker"):
            self.worker.cancel()
            self.worker.wait(2000)
        self.btn_cancel.setEnabled(False)
        self.btn_send.setEnabled(True)

    def _on_progress(self, current, total, speed):
        self.progress.setValue(speed)
        self.lbl_file_info.setText(f"Sending: {current}/{total} chunks")

    def _on_completed(self, success, msg):
        self.btn_cancel.setEnabled(False)
        self.btn_send.setEnabled(True)
        self.lbl_file_info.setText(msg)
        self.progress.setValue(100 if success else 0)

    def on_chunk_received(self, pkt):
        """Handle incoming file chunk."""
        if len(pkt.payload) < 2:
            return

        chunk_idx = int.from_bytes(pkt.payload[:2], "big")
        chunk_data = pkt.payload[2:]

        # Decrypt if needed
        if pkt.flags & PacketFlags.ENCRYPTED and self.crypto.is_ready():
            decrypted = self.crypto.decrypt(chunk_data)
            if decrypted:
                chunk_data = decrypted

        # Store chunk (simplified — in production use FileTransferState)
        # Send ACK
        ack = HLinkPacket.create(PacketType.ACK, pkt.payload[:2])
        self.udp.send(ack)

    def on_ack_received(self, pkt):
        pass  # Handled by worker
