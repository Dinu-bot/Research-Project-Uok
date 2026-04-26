# ============================================================================
#  styles.py  —  Tactical-dark theme stylesheet
# ============================================================================
from _future_ import annotations

from PyQt6.QtWidgets import QApplication

from ..core import config


_QSS = f"""
QMainWindow, QWidget {{
    background-color: {config.BG_COLOR};
    color: {config.ACCENT_GREEN};
    font-family: "Consolas", "Menlo", "DejaVu Sans Mono", monospace;
    font-size: 11px;
}}
QFrame#sidebar {{
    background-color: {config.SIDEBAR_COLOR};
    border-radius: 5px;
}}
QLabel {{
    color: {config.ACCENT_GREEN};
    font-weight: bold;
}}
QLabel#statusValue {{
    color: white;
}}
QProgressBar {{
    border: 1px solid #333;
    border-radius: 2px;
    background: #050505;
    text-align: center;
    height: 18px;
}}
QProgressBar::chunk {{
    background-color: {config.ACCENT_GREEN};
}}
QTextEdit, QLineEdit, QPlainTextEdit {{
    background-color: #050505;
    border: 1px solid #222;
    color: {config.ACCENT_GREEN};
    padding: 4px;
    selection-background-color: {config.ACCENT_GREEN};
    selection-color: black;
}}
QPushButton {{
    background-color: #1A1A1B;
    border: 1px solid #333;
    color: #888;
    padding: 8px 12px;
}}
QPushButton:hover {{
    border-color: {config.ACCENT_GREEN};
    color: white;
}}
QPushButton:pressed {{
    background-color: #2A2A2B;
}}
QPushButton#primary {{
    background-color: {config.ACCENT_GREEN};
    color: black;
    font-weight: bold;
}}
QPushButton#emergency {{
    background-color: {config.DANGER_RED};
    color: white;
    font-weight: bold;
}}
QTabWidget::pane {{
    border: 1px solid #222;
    background-color: {config.BG_COLOR};
}}
QTabBar::tab {{
    background: {config.SIDEBAR_COLOR};
    color: #888;
    padding: 8px 14px;
    border: 1px solid #222;
    border-bottom: none;
}}
QTabBar::tab:selected {{
    background: {config.BG_COLOR};
    color: {config.ACCENT_GREEN};
}}
"""


def apply_tactical_theme(app: QApplication) -> None:
    app.setStyleSheet(_QSS)