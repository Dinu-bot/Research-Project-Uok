#!/usr/bin/env python3
"""
SnakeLink v2.0 — Tactical Communication Software
PyQt6 Desktop Application
"""

import sys
import os
import signal
from PyQt6.QtWidgets import QApplication
from PyQt6.QtCore import Qt

# Add project root to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from ui.main_window import MainWindow
from config import AppConfig


def main():
    # Enable high DPI scaling
    QApplication.setHighDpiScaleFactorRoundingPolicy(
        Qt.HighDpiScaleFactorRoundingPolicy.PassThrough
    )

    app = QApplication(sys.argv)
    app.setApplicationName("SnakeLink")
    app.setApplicationVersion("2.0.0")
    app.setOrganizationName("UoK-Research")

    # Load configuration
    config = AppConfig()

    # Create main window
    window = MainWindow(config)
    window.show()

    # Handle Ctrl+C gracefully
    signal.signal(signal.SIGINT, lambda *args: app.quit())

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
