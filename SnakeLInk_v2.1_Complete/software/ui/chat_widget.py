"""
Chat Widget — WhatsApp-style message bubbles with delivery confirmation
"""

from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QTextEdit, QLineEdit,
    QPushButton, QScrollArea, QFrame, QLabel, QSizePolicy
)
from PyQt6.QtCore import Qt, pyqtSlot
from PyQt6.QtGui import QFont

from config import AppConfig
from udp_client import UDPClient
from crypto_manager import CryptoManager
from hlink_protocol import HLinkPacket, PacketType, PacketFlags


class MessageBubble(QFrame):
    """Individual message bubble widget."""

    def __init__(self, text: str, sent: bool = True, 
                 delivery_status: str = "sent"):
        super().__init__()
        self.setObjectName("msgBubble")
        self.sent = sent
        self.delivery_status = delivery_status

        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 8, 12, 8)

        self.lbl_text = QLabel(text)
        self.lbl_text.setWordWrap(True)
        self.lbl_text.setFont(QFont("Segoe UI", 11))
        layout.addWidget(self.lbl_text)

        # Status row
        status_layout = QHBoxLayout()
        status_layout.addStretch()

        self.lbl_time = QLabel("12:00")
        self.lbl_time.setObjectName("msgTime")
        status_layout.addWidget(self.lbl_time)

        self.lbl_status = QLabel(delivery_status)
        self.lbl_status.setObjectName("msgStatus")
        status_layout.addWidget(self.lbl_status)

        layout.addLayout(status_layout)

        self._setup_style()

    def _setup_style(self):
        if self.sent:
            self.setStyleSheet("""
                #msgBubble {
                    background-color: #0f3460;
                    border-radius: 12px;
                    border-bottom-right-radius: 4px;
                    margin-left: 64px;
                    margin-right: 8px;
                }
                QLabel { color: #e9ecef; }
                #msgTime { color: #a5d8ff; font-size: 10px; }
                #msgStatus { color: #51cf66; font-size: 10px; }
            """)
        else:
            self.setStyleSheet("""
                #msgBubble {
                    background-color: #2d3436;
                    border-radius: 12px;
                    border-bottom-left-radius: 4px;
                    margin-left: 8px;
                    margin-right: 64px;
                }
                QLabel { color: #e9ecef; }
                #msgTime { color: #a5d8ff; font-size: 10px; }
                #msgStatus { color: #74c0fc; font-size: 10px; }
            """)

    def set_status(self, status: str):
        self.delivery_status = status
        self.lbl_status.setText(status)


class ChatWidget(QWidget):
    """Chat interface with message bubbles."""

    def __init__(self, config: AppConfig, udp: UDPClient, crypto: CryptoManager):
        super().__init__()
        self.config = config
        self.udp = udp
        self.crypto = crypto
        self.messages = []
        self.pending_acks = {}

        self._setup_ui()

    def _setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(12)

        # Messages area
        self.scroll = QScrollArea()
        self.scroll.setWidgetResizable(True)
        self.scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)

        self.msg_container = QWidget()
        self.msg_layout = QVBoxLayout(self.msg_container)
        self.msg_layout.setAlignment(Qt.AlignmentFlag.AlignTop)
        self.msg_layout.setSpacing(8)
        self.msg_layout.addStretch()

        self.scroll.setWidget(self.msg_container)
        layout.addWidget(self.scroll)

        # Input area
        input_layout = QHBoxLayout()

        self.txt_input = QLineEdit()
        self.txt_input.setPlaceholderText("Type message...")
        self.txt_input.setFixedHeight(40)
        self.txt_input.returnPressed.connect(self._send_message)
        input_layout.addWidget(self.txt_input)

        self.btn_send = QPushButton("Send")
        self.btn_send.setFixedSize(80, 40)
        self.btn_send.clicked.connect(self._send_message)
        input_layout.addWidget(self.btn_send)

        layout.addLayout(input_layout)

    def _send_message(self):
        text = self.txt_input.text().strip()
        if not text:
            return

        # Encrypt if enabled
        payload = text.encode("utf-8")
        if self.config.high_security_mode and self.crypto.is_ready():
            encrypted = self.crypto.encrypt(payload)
            if encrypted:
                payload = encrypted

        # Truncate to max payload
        if len(payload) > 240:
            payload = payload[:240]

        pkt = HLinkPacket.create(PacketType.TEXT, payload)
        if self.config.high_security_mode:
            pkt.flags |= PacketFlags.ENCRYPTED

        self.udp.send(pkt)

        # Add to UI
        bubble = MessageBubble(text, sent=True, delivery_status="✓")
        self.msg_layout.insertWidget(self.msg_layout.count() - 1, bubble)
        self.messages.append((pkt.seq, bubble))
        self.pending_acks[pkt.seq] = bubble

        self.txt_input.clear()
        self.scroll.verticalScrollBar().setValue(
            self.scroll.verticalScrollBar().maximum()
        )

    @pyqtSlot(object)
    def on_message_received(self, pkt):
        """Handle incoming text message."""
        payload = pkt.payload

        # Decrypt if needed
        if pkt.flags & PacketFlags.ENCRYPTED and self.crypto.is_ready():
            decrypted = self.crypto.decrypt(payload)
            if decrypted:
                payload = decrypted

        try:
            text = payload.decode("utf-8")
        except:
            text = "[Binary data]"

        bubble = MessageBubble(text, sent=False, delivery_status="✓✓")
        self.msg_layout.insertWidget(self.msg_layout.count() - 1, bubble)

        # Send ACK
        ack = HLinkPacket.create(PacketType.ACK, pkt.seq.to_bytes(2, "big"))
        self.udp.send(ack)

        self.scroll.verticalScrollBar().setValue(
            self.scroll.verticalScrollBar().maximum()
        )

    @pyqtSlot(object)
    def on_ack_received(self, pkt):
        """Handle ACK for sent message."""
        if len(pkt.payload) >= 2:
            seq = int.from_bytes(pkt.payload[:2], "big")
            if seq in self.pending_acks:
                self.pending_acks[seq].set_status("✓✓")
                del self.pending_acks[seq]
