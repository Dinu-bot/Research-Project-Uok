"""
Voice Communication Widget — Push-to-Talk
"""

import struct
from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton,
    QProgressBar, QFrame
)
from PyQt6.QtCore import Qt, QTimer, pyqtSlot
from PyQt6.QtGui import QFont

from config import AppConfig
from udp_client import UDPClient
from hlink_protocol import HLinkPacket, PacketType


class VoiceWidget(QWidget):
    """Push-to-Talk voice interface."""

    def __init__(self, config: AppConfig, udp: UDPClient):
        super().__init__()
        self.config = config
        self.udp = udp
        self.ptt_active = False

        self._setup_ui()
        self._setup_ptt()

    def _setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(16)

        # Header
        header = QLabel("Voice Communication")
        header.setFont(QFont("Segoe UI", 16, QFont.Weight.Bold))
        layout.addWidget(header)

        # Status
        self.lbl_status = QLabel("Ready — Press SPACE or hold button to talk")
        self.lbl_status.setObjectName("statusLabel")
        layout.addWidget(self.lbl_status)

        # Level meter
        self.level_meter = QProgressBar()
        self.level_meter.setRange(0, 100)
        self.level_meter.setValue(0)
        self.level_meter.setTextVisible(False)
        self.level_meter.setFixedHeight(24)
        layout.addWidget(self.level_meter)

        # PTT Button
        self.btn_ptt = QPushButton("PUSH TO TALK")
        self.btn_ptt.setObjectName("pttBtn")
        self.btn_ptt.setFixedHeight(120)
        self.btn_ptt.setCheckable(True)
        self.btn_ptt.pressed.connect(self._ptt_pressed)
        self.btn_ptt.released.connect(self._ptt_released)
        layout.addWidget(self.btn_ptt)

        # Codec info
        self.lbl_codec = QLabel("Codec: Opus (auto-selected)")
        layout.addWidget(self.lbl_codec)

        layout.addStretch()

        self.setStyleSheet("""
            #statusLabel {
                color: #a5d8ff;
                font-size: 14px;
            }
            #pttBtn {
                background-color: #2d3436;
                color: #e9ecef;
                font-size: 24px;
                font-weight: bold;
                border: 4px solid #e94560;
                border-radius: 12px;
            }
            #pttBtn:pressed, #pttBtn:checked {
                background-color: #c92a2a;
                border-color: #ff6b6b;
            }
            QProgressBar {
                background-color: #1a1a2e;
                border: 1px solid #0f3460;
                border-radius: 4px;
            }
            QProgressBar::chunk {
                background-color: #51cf66;
                border-radius: 4px;
            }
        """)

    def _setup_ptt(self):
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)

    def keyPressEvent(self, event):
        if event.key() == Qt.Key.Key_Space and not self.ptt_active:
            self._ptt_pressed()
        else:
            super().keyPressEvent(event)

    def keyReleaseEvent(self, event):
        if event.key() == Qt.Key.Key_Space and self.ptt_active:
            self._ptt_released()
        else:
            super().keyReleaseEvent(event)

    def _ptt_pressed(self):
        self.ptt_active = True
        self.btn_ptt.setChecked(True)
        self.lbl_status.setText("TRANSMITTING...")
        self.lbl_status.setStyleSheet("color: #ff6b6b; font-size: 14px;")

        # Send voice request
        req = HLinkPacket.create(PacketType.VOICE_REQ, b"CALL")
        self.udp.send(req)

    def _ptt_released(self):
        self.ptt_active = False
        self.btn_ptt.setChecked(False)
        self.lbl_status.setText("Ready — Press SPACE or hold button to talk")
        self.lbl_status.setStyleSheet("color: #a5d8ff; font-size: 14px;")
        self.level_meter.setValue(0)

    @pyqtSlot(object)
    def on_voice_received(self, pkt):
        """Handle incoming voice frame."""
        # In production: decode and play audio
        self.level_meter.setValue(50)  # Visual feedback
        QTimer.singleShot(100, lambda: self.level_meter.setValue(0))
