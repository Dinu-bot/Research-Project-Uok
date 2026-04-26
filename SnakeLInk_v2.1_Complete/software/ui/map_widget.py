"""
Offline Map Widget — folium-based with pre-downloaded tiles
"""

from PyQt6.QtWidgets import QWidget, QVBoxLayout, QLabel
from PyQt6.QtCore import Qt, pyqtSlot

from config import AppConfig


class MapWidget(QWidget):
    """Offline map display."""

    def __init__(self, config: AppConfig):
        super().__init__()
        self.config = config
        self.own_lat = config.default_lat
        self.own_lon = config.default_lon
        self.other_lat = None
        self.other_lon = None

        self._setup_ui()

    def _setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)

        header = QLabel("GPS / Map")
        header.setStyleSheet("font-size: 16px; font-weight: bold; color: #e9ecef;")
        layout.addWidget(header)

        # In production: embed QWebEngineView with folium map
        # For now, show coordinates
        self.lbl_coords = QLabel(f"Own: {self.own_lat:.4f}, {self.own_lon:.4f}")
        self.lbl_coords.setStyleSheet("color: #a5d8ff; font-size: 14px;")
        layout.addWidget(self.lbl_coords)

        self.lbl_other = QLabel("Other: No data")
        self.lbl_other.setStyleSheet("color: #ffd43b; font-size: 14px;")
        layout.addWidget(self.lbl_other)

        layout.addStretch()

    @pyqtSlot(object)
    def on_gps_received(self, pkt):
        """Handle incoming GPS packet."""
        if len(pkt.payload) >= 8:
            import struct
            lat = struct.unpack("<f", pkt.payload[:4])[0]
            lon = struct.unpack("<f", pkt.payload[4:8])[0]
            self.other_lat = lat
            self.other_lon = lon
            self.lbl_other.setText(f"Other: {lat:.4f}, {lon:.4f}")
