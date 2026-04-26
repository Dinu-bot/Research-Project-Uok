"""
Real-time Dashboard — Link health, telemetry, statistics
"""

import struct
from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QProgressBar,
    QFrame, QGridLayout
)
from PyQt6.QtCore import Qt, pyqtSlot
from PyQt6.QtGui import QFont

from config import AppConfig
from udp_client import UDPClient


class DashboardWidget(QWidget):
    """Link health dashboard."""

    def __init__(self, config: AppConfig, udp: UDPClient):
        super().__init__()
        self.config = config
        self.udp = udp

        self._setup_ui()

    def _setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(16)

        header = QLabel("Link Dashboard")
        header.setFont(QFont("Segoe UI", 16, QFont.Weight.Bold))
        layout.addWidget(header)

        # Signal bars grid
        grid = QGridLayout()
        grid.setSpacing(12)

        # Wi-Fi
        grid.addWidget(QLabel("Wi-Fi RSSI:"), 0, 0)
        self.bar_wifi = QProgressBar()
        self.bar_wifi.setRange(-90, -30)
        self.bar_wifi.setValue(-80)
        grid.addWidget(self.bar_wifi, 0, 1)
        self.lbl_wifi = QLabel("-80 dBm")
        grid.addWidget(self.lbl_wifi, 0, 2)

        # FSO
        grid.addWidget(QLabel("FSO Signal:"), 1, 0)
        self.bar_fso = QProgressBar()
        self.bar_fso.setRange(0, 3300)
        self.bar_fso.setValue(1650)
        grid.addWidget(self.bar_fso, 1, 1)
        self.lbl_fso = QLabel("1.65V")
        grid.addWidget(self.lbl_fso, 1, 2)

        # LoRa
        grid.addWidget(QLabel("LoRa RSSI:"), 2, 0)
        self.bar_lora = QProgressBar()
        self.bar_lora.setRange(-120, -30)
        self.bar_lora.setValue(-100)
        grid.addWidget(self.bar_lora, 2, 1)
        self.lbl_lora = QLabel("-100 dBm")
        grid.addWidget(self.lbl_lora, 2, 2)

        layout.addLayout(grid)

        # Stats
        stats_frame = QFrame()
        stats_frame.setObjectName("statsFrame")
        stats_layout = QGridLayout(stats_frame)

        self.lbl_tx = QLabel("TX: 0 pkts")
        self.lbl_rx = QLabel("RX: 0 pkts")
        self.lbl_errors = QLabel("Errors: 0")
        self.lbl_uptime = QLabel("Uptime: 0s")

        stats_layout.addWidget(self.lbl_tx, 0, 0)
        stats_layout.addWidget(self.lbl_rx, 0, 1)
        stats_layout.addWidget(self.lbl_errors, 1, 0)
        stats_layout.addWidget(self.lbl_uptime, 1, 1)

        layout.addWidget(stats_frame)

        # Battery
        self.lbl_bat = QLabel("Battery: --")
        layout.addWidget(self.lbl_bat)

        layout.addStretch()

        self.setStyleSheet("""
            #statsFrame {
                background-color: #16213e;
                border-radius: 8px;
                padding: 12px;
            }
            QLabel { color: #e9ecef; }
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

    @pyqtSlot(object)
    def on_link_status(self, pkt):
        """Handle link status packet."""
        if len(pkt.payload) >= 4:
            import struct
            fso_v = struct.unpack("<f", pkt.payload[:4])[0]
            self.bar_fso.setValue(int(fso_v * 1000))
            self.lbl_fso.setText(f"{fso_v:.2f}V")

    @pyqtSlot(object)
    def on_battery_status(self, pkt):
        """Handle battery packet."""
        if len(pkt.payload) >= 1:
            pct = pkt.payload[4] if len(pkt.payload) > 4 else 0
            self.lbl_bat.setText(f"Other Battery: {pct}%")

    def update_stats(self, stats: dict):
        self.lbl_tx.setText(f"TX: {stats.get('tx', 0)} pkts")
        self.lbl_rx.setText(f"RX: {stats.get('rx', 0)} pkts")
        self.lbl_errors.setText(f"Errors: {stats.get('errors', 0)}")
