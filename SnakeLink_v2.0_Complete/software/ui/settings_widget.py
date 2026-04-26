"""
Settings Widget — Device config, thresholds, security
"""

from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QLineEdit,
    QPushButton, QCheckBox, QSpinBox, QDoubleSpinBox,
    QGroupBox, QFormLayout, QFileDialog
)
from PyQt6.QtCore import Qt

from config import AppConfig


class SettingsWidget(QWidget):
    """Application settings interface."""

    def __init__(self, config: AppConfig):
        super().__init__()
        self.config = config
        self._setup_ui()
        self._load_values()

    def _setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(16)

        header = QLabel("Settings")
        header.setStyleSheet("font-size: 16px; font-weight: bold; color: #e9ecef;")
        layout.addWidget(header)

        # Device settings
        device_group = QGroupBox("Device")
        device_layout = QFormLayout(device_group)

        self.txt_name = QLineEdit()
        device_layout.addRow("Device Name:", self.txt_name)

        self.txt_ip = QLineEdit()
        device_layout.addRow("Transceiver IP:", self.txt_ip)

        layout.addWidget(device_group)

        # Security settings
        sec_group = QGroupBox("Security")
        sec_layout = QFormLayout(sec_group)

        self.chk_high_sec = QCheckBox("High Security Mode (E2E Encryption)")
        sec_layout.addRow(self.chk_high_sec)

        self.btn_gen_key = QPushButton("Generate New Key")
        self.btn_gen_key.clicked.connect(self._gen_key)
        sec_layout.addRow(self.btn_gen_key)

        self.lbl_fingerprint = QLabel("Key: --")
        sec_layout.addRow("Fingerprint:", self.lbl_fingerprint)

        layout.addWidget(sec_group)

        # Thresholds
        thresh_group = QGroupBox("Link Thresholds")
        thresh_layout = QFormLayout(thresh_group)

        self.spin_wifi_deg = QSpinBox()
        self.spin_wifi_deg.setRange(-100, -30)
        thresh_layout.addRow("Wi-Fi Degrade (dBm):", self.spin_wifi_deg)

        self.spin_wifi_rec = QSpinBox()
        self.spin_wifi_rec.setRange(-100, -30)
        thresh_layout.addRow("Wi-Fi Recover (dBm):", self.spin_wifi_rec)

        self.spin_fso_deg = QDoubleSpinBox()
        self.spin_fso_deg.setRange(0.0, 3.3)
        self.spin_fso_deg.setDecimals(2)
        thresh_layout.addRow("FSO Degrade (V):", self.spin_fso_deg)

        self.spin_fso_rec = QDoubleSpinBox()
        self.spin_fso_rec.setRange(0.0, 3.3)
        self.spin_fso_rec.setDecimals(2)
        thresh_layout.addRow("FSO Recover (V):", self.spin_fso_rec)

        layout.addWidget(thresh_group)

        # Save button
        self.btn_save = QPushButton("Save Settings")
        self.btn_save.clicked.connect(self._save)
        layout.addWidget(self.btn_save)

        layout.addStretch()

        self.setStyleSheet("""
            QGroupBox {
                color: #e9ecef;
                border: 1px solid #0f3460;
                border-radius: 8px;
                margin-top: 12px;
                padding-top: 12px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 8px;
                padding: 0 4px;
            }
            QLabel { color: #e9ecef; }
            QLineEdit, QSpinBox, QDoubleSpinBox {
                background-color: #16213e;
                color: #e9ecef;
                border: 1px solid #0f3460;
                border-radius: 4px;
                padding: 4px;
            }
            QCheckBox { color: #e9ecef; }
            QCheckBox::indicator {
                width: 18px;
                height: 18px;
            }
        """)

    def _load_values(self):
        self.txt_name.setText(self.config.device_name)
        self.txt_ip.setText(self.config.transceiver_ip)
        self.chk_high_sec.setChecked(self.config.high_security_mode)
        self.spin_wifi_deg.setValue(self.config.wifi_rssi_degrade)
        self.spin_wifi_rec.setValue(self.config.wifi_rssi_recover)
        self.spin_fso_deg.setValue(self.config.fso_voltage_degrade)
        self.spin_fso_rec.setValue(self.config.fso_voltage_recover)

    def _save(self):
        self.config.device_name = self.txt_name.text()
        self.config.transceiver_ip = self.txt_ip.text()
        self.config.high_security_mode = self.chk_high_sec.isChecked()
        self.config.wifi_rssi_degrade = self.spin_wifi_deg.value()
        self.config.wifi_rssi_recover = self.spin_wifi_rec.value()
        self.config.fso_voltage_degrade = self.spin_fso_deg.value()
        self.config.fso_voltage_recover = self.spin_fso_rec.value()
        self.config.save()

    def _gen_key(self):
        import os
        import base64
        key = os.urandom(32)
        self.config.set_key_bytes(key)
        fingerprint = key[:2].hex().upper()
        self.lbl_fingerprint.setText(f"Key: {fingerprint}")
