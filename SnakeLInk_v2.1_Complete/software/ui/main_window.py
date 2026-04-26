"""
SnakeLink Main Window — Dark Tactical Theme
"""

import os
from PyQt6.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QPushButton, QStackedWidget, QFrame,
    QSizePolicy, QMessageBox
)
from PyQt6.QtCore import Qt, QTimer, pyqtSlot
from PyQt6.QtGui import QFont, QPalette, QColor, QIcon

from config import AppConfig
from udp_client import UDPClient
from crypto_manager import CryptoManager
from ui.chat_widget import ChatWidget
from ui.voice_widget import VoiceWidget
from ui.files_widget import FilesWidget
from ui.map_widget import MapWidget
from ui.dashboard_widget import DashboardWidget
from ui.settings_widget import SettingsWidget


class MainWindow(QMainWindow):
    """Main application window with tactical dark theme."""

    def __init__(self, config: AppConfig):
        super().__init__()
        self.config = config
        self.config.load()

        # Managers
        self.udp = UDPClient(
            local_port=self.config.local_port,
            remote_port=self.config.transceiver_port
        )
        self.crypto = CryptoManager()
        if self.config.encryption_key:
            self.crypto.set_key(self.config.get_key_bytes())

        self._setup_ui()
        self._setup_styles()
        self._connect_signals()

        # Connection status timer
        self._status_timer = QTimer(self)
        self._status_timer.timeout.connect(self._update_status)
        self._status_timer.start(1000)

        # Try auto-connect
        self._try_connect()

    def _setup_ui(self):
        self.setWindowTitle("SnakeLink v2.0 — Tactical Communication")
        self.setMinimumSize(1200, 800)

        central = QWidget()
        self.setCentralWidget(central)
        layout = QVBoxLayout(central)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        # === TOP STATUS BAR ===
        self.status_bar = self._create_status_bar()
        layout.addWidget(self.status_bar)

        # === MAIN CONTENT ===
        content = QHBoxLayout()
        content.setSpacing(0)

        # Left sidebar
        self.sidebar = self._create_sidebar()
        content.addWidget(self.sidebar)

        # Content area
        self.stack = QStackedWidget()
        self.stack.setObjectName("contentArea")

        # Create pages
        self.chat_page = ChatWidget(self.config, self.udp, self.crypto)
        self.voice_page = VoiceWidget(self.config, self.udp)
        self.files_page = FilesWidget(self.config, self.udp, self.crypto)
        self.map_page = MapWidget(self.config)
        self.dashboard_page = DashboardWidget(self.config, self.udp)
        self.settings_page = SettingsWidget(self.config)

        self.stack.addWidget(self.chat_page)      # 0
        self.stack.addWidget(self.voice_page)     # 1
        self.stack.addWidget(self.files_page)     # 2
        self.stack.addWidget(self.map_page)       # 3
        self.stack.addWidget(self.dashboard_page) # 4
        self.stack.addWidget(self.settings_page)  # 5

        content.addWidget(self.stack, 1)
        layout.addLayout(content, 1)

    def _create_status_bar(self) -> QFrame:
        bar = QFrame()
        bar.setObjectName("statusBar")
        bar.setFixedHeight(48)
        bar.setFrameShape(QFrame.Shape.StyledPanel)

        layout = QHBoxLayout(bar)
        layout.setContentsMargins(16, 4, 16, 4)

        # Device name
        self.lbl_device = QLabel("SnakeLink")
        self.lbl_device.setObjectName("deviceLabel")
        layout.addWidget(self.lbl_device)

        layout.addStretch()

        # Connection status
        self.lbl_conn = QLabel("DISCONNECTED")
        self.lbl_conn.setObjectName("connLabel")
        layout.addWidget(self.lbl_conn)

        # Active link
        self.lbl_link = QLabel("FSO")
        self.lbl_link.setObjectName("linkLabel")
        layout.addWidget(self.lbl_link)

        # FSO signal strength indicator
        self.lbl_fso_status = QLabel("●")
        self.lbl_fso_status.setObjectName("fsoStatus")
        self.lbl_fso_status.setStyleSheet("color: #51cf66; font-size: 14px;")
        layout.addWidget(self.lbl_fso_status)

        # Signal strength
        self.lbl_signal = QLabel("●●●")
        self.lbl_signal.setObjectName("signalLabel")
        layout.addWidget(self.lbl_signal)

        # Battery
        self.lbl_battery = QLabel("100%")
        self.lbl_battery.setObjectName("batteryLabel")
        layout.addWidget(self.lbl_battery)

        layout.addStretch()

        # Emergency button
        self.btn_emergency = QPushButton("EMERGENCY")
        self.btn_emergency.setObjectName("emergencyBtn")
        self.btn_emergency.setFixedSize(120, 36)
        self.btn_emergency.clicked.connect(self._on_emergency)
        layout.addWidget(self.btn_emergency)

        return bar

    def _create_sidebar(self) -> QFrame:
        sidebar = QFrame()
        sidebar.setObjectName("sidebar")
        sidebar.setFixedWidth(80)
        sidebar.setFrameShape(QFrame.Shape.StyledPanel)

        layout = QVBoxLayout(sidebar)
        layout.setSpacing(8)
        layout.setContentsMargins(8, 16, 8, 16)

        buttons = [
            ("Chat", 0),
            ("Voice", 1),
            ("Files", 2),
            ("Map", 3),
            ("Dash", 4),
            ("Set", 5),
        ]

        for text, idx in buttons:
            btn = QPushButton(text)
            btn.setObjectName("navBtn")
            btn.setFixedSize(64, 64)
            btn.setCheckable(True)
            btn.clicked.connect(lambda checked, i=idx: self._switch_page(i))
            layout.addWidget(btn)
            if idx == 0:
                btn.setChecked(True)
                self._active_nav = btn

        layout.addStretch()
        return sidebar

    def _switch_page(self, index: int):
        self.stack.setCurrentIndex(index)
        # Update button states
        for btn in self.sidebar.findChildren(QPushButton):
            btn.setChecked(False)
        sender = self.sender()
        if sender:
            sender.setChecked(True)

    def _setup_styles(self):
        self.setStyleSheet("""
            QMainWindow {
                background-color: #1a1a2e;
            }
            #statusBar {
                background-color: #16213e;
                border-bottom: 2px solid #0f3460;
            }
            #deviceLabel {
                color: #e94560;
                font-size: 16px;
                font-weight: bold;
            }
            #connLabel {
                color: #ff6b6b;
                font-size: 12px;
                font-weight: bold;
            }
            #connLabel[connected="true"] {
                color: #51cf66;
            }
            #linkLabel {
                color: #ffd43b;
                font-size: 14px;
                font-weight: bold;
                padding: 2px 12px;
                background-color: #0f3460;
                border-radius: 4px;
            }
            #signalLabel {
                color: #51cf66;
                font-size: 14px;
            }
            #batteryLabel {
                color: #74c0fc;
                font-size: 12px;
            }
            #emergencyBtn {
                background-color: #c92a2a;
                color: white;
                font-weight: bold;
                border: none;
                border-radius: 4px;
            }
            #emergencyBtn:hover {
                background-color: #e03131;
            }
            #emergencyBtn:pressed {
                background-color: #a61e1e;
            }
            #sidebar {
                background-color: #16213e;
                border-right: 2px solid #0f3460;
            }
            #navBtn {
                background-color: #1a1a2e;
                color: #a5d8ff;
                font-size: 11px;
                font-weight: bold;
                border: 2px solid #0f3460;
                border-radius: 8px;
            }
            #navBtn:hover {
                background-color: #0f3460;
                border-color: #e94560;
            }
            #navBtn:checked {
                background-color: #e94560;
                color: white;
                border-color: #e94560;
            }
            #contentArea {
                background-color: #1a1a2e;
            }
            QLabel {
                color: #e9ecef;
            }
            QPushButton {
                color: #e9ecef;
            }
            QTextEdit, QLineEdit {
                background-color: #16213e;
                color: #e9ecef;
                border: 1px solid #0f3460;
                border-radius: 4px;
            }
            QScrollBar:vertical {
                background-color: #16213e;
                width: 12px;
            }
            QScrollBar::handle:vertical {
                background-color: #0f3460;
                border-radius: 6px;
            }
        """)

    def _connect_signals(self):
        self.udp.packet_received.connect(self._on_packet)
        self.udp.connected.connect(self._on_connection_changed)
        self.udp.error.connect(self._on_error)

    def _try_connect(self):
        if self.config.transceiver_ip:
            self.udp.start(self.config.transceiver_ip)

    @pyqtSlot(object)
    def _on_packet(self, pkt):
        """Handle incoming packet."""
        from hlink_protocol import PacketType

        if pkt.type == PacketType.TEXT:
            self.chat_page.on_message_received(pkt)
        elif pkt.type == PacketType.FILE_CHUNK:
            self.files_page.on_chunk_received(pkt)
        elif pkt.type == PacketType.ACK:
            self.files_page.on_ack_received(pkt)
        elif pkt.type == PacketType.VOICE:
            self.voice_page.on_voice_received(pkt)
        elif pkt.type == PacketType.EMERGENCY:
            self._on_emergency_received(pkt)
        elif pkt.type == PacketType.LINK_STATUS:
            self.dashboard_page.on_link_status(pkt)
        elif pkt.type == PacketType.BATTERY:
            self.dashboard_page.on_battery_status(pkt)
        elif pkt.type == PacketType.GPS:
            self.map_page.on_gps_received(pkt)

    @pyqtSlot(bool)
    def _on_connection_changed(self, connected: bool):
        if connected:
            self.lbl_conn.setText("TRANSCEIVER CONNECTED ✓")
            self.lbl_conn.setProperty("connected", "true")
            self.lbl_link.setText("FSO")
            self.lbl_link.setStyleSheet("background-color: #e94560; color: white;")
        else:
            self.lbl_conn.setText("DISCONNECTED")
            self.lbl_conn.setProperty("connected", "false")
            self.lbl_link.setText("---")
            self.lbl_link.setStyleSheet("")
        self.lbl_conn.style().unpolish(self.lbl_conn)
        self.lbl_conn.style().polish(self.lbl_conn)

    @pyqtSlot(str)
    def _on_error(self, msg: str):
        print(f"[UDP Error] {msg}")

    def _update_status(self):
        """Periodic status update."""
        stats = self.udp.get_stats()
        self.dashboard_page.update_stats(stats)

    def update_link_status(self, link_type: str, signal_quality: float):
        """Update status bar with current active link."""
        link_names = {"FSO": "FSO", "WIFI": "WIFI", "LORA": "LORA"}
        link_colors = {"FSO": "#e94560", "WIFI": "#74c0fc", "LORA": "#51cf66"}

        name = link_names.get(link_type, "---")
        color = link_colors.get(link_type, "#a5d8ff")

        self.lbl_link.setText(name)
        self.lbl_link.setStyleSheet(f"color: {color}; font-size: 14px; font-weight: bold; padding: 2px 12px; background-color: #0f3460; border-radius: 4px;")

        # Update FSO indicator
        if link_type == "FSO":
            self.lbl_fso_status.setStyleSheet("color: #51cf66; font-size: 14px;")
        else:
            self.lbl_fso_status.setStyleSheet("color: #2d3436; font-size: 14px;")

    def _on_emergency(self):
        reply = QMessageBox.warning(
            self, "EMERGENCY BROADCAST",
            "Send emergency broadcast to ALL links?\n\n"
            "This will trigger alarms on the receiving device.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No
        )
        if reply == QMessageBox.StandardButton.Yes:
            from hlink_protocol import HLinkPacket, PacketType, PacketFlags
            import struct

            payload = struct.pack("<ff", self.config.default_lat, self.config.default_lon)
            payload += b"EMERGENCY"

            pkt = HLinkPacket.create(
                PacketType.EMERGENCY,
                payload,
                PacketFlags.BROADCAST | PacketFlags.PRIORITY
            )
            self.udp.send(pkt)

    def _on_emergency_received(self, pkt):
        QMessageBox.critical(
            self, "EMERGENCY RECEIVED",
            "EMERGENCY BROADCAST RECEIVED!\n\n"
            "Check GPS coordinates immediately."
        )

    def closeEvent(self, event):
        self.udp.stop()
        self.config.save()
        event.accept()
