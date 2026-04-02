import sys
import random
import psutil  # IMPORTANT: Run 'pip install psutil' for real laptop battery sensing
import pyqtgraph as pg
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                             QHBoxLayout, QLabel, QPushButton, QTextEdit, 
                             QLineEdit, QProgressBar, QFrame)
from PyQt6.QtCore import Qt, QTimer, QDateTime

# --- TACTICAL THEME ---
BG_COLOR = "#0A0A0B"
ACCENT_GREEN = "#00FF41"
SIDEBAR_COLOR = "#121213"

class SnakeLinkApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("SnakeLink Tactical Dashboard v2.7")
        self.setMinimumSize(1200, 800)
        
        # Hardware Simulation States
        self.remote_charging = False 
        self.remote_val = 75
        
        # --- STYLESHEET ---
        self.setStyleSheet(f"""
            QMainWindow {{ background-color: {BG_COLOR}; }}
            QLabel {{ color: {ACCENT_GREEN}; font-weight: bold; font-size: 11px; }}
            QProgressBar {{ 
                border: 1px solid #333; 
                border-radius: 2px; 
                text-align: center; 
                background: #050505; 
                font-weight: bold;
                height: 18px;
            }}
            QProgressBar::chunk {{ background-color: {ACCENT_GREEN}; }}
            QTextEdit, QLineEdit {{ 
                background-color: #050505; 
                border: 1px solid #222; 
                color: {ACCENT_GREEN}; 
                padding: 5px;
            }}
            QPushButton {{ 
                background-color: #1A1A1B; 
                border: 1px solid #333; 
                color: #888; 
                padding: 10px;
            }}
            QPushButton:hover {{ border-color: {ACCENT_GREEN}; color: white; }}
        """)
        
        self.initUI()
        
        # Telemetry Timer - Set to 10 seconds for stable status updates
        self.status_timer = QTimer()
        self.status_timer.timeout.connect(self.update_telemetry)
        self.status_timer.start(2000) # Faster UI response, but logic handles the 10s feel

    def initUI(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QHBoxLayout(central_widget)
        main_layout.setContentsMargins(10, 10, 10, 10)

        # 1. SIDEBAR (Fixed Button Connections)
        sidebar = QFrame()
        sidebar.setFixedWidth(70)
        sidebar.setStyleSheet(f"background-color: {SIDEBAR_COLOR}; border-radius: 5px;")
        sidebar_layout = QVBoxLayout(sidebar)
        
        icons = [("💬", "Chat"), ("📊", "Analytics"), ("🛰️", "Nodes"), ("⚙️", "Config")]
        for icon, name in icons:
            btn = QPushButton(icon)
            btn.setFixedSize(50, 50)
            btn.setToolTip(name)
            # FIX: Ensure buttons trigger a log action
            btn.clicked.connect(lambda _, n=name: self.add_log(f"System: Accessing {n} module..."))
            sidebar_layout.addWidget(btn)
            
        sidebar_layout.addStretch()
        main_layout.addWidget(sidebar)

        # 2. CENTER CONTENT
        mid_container = QVBoxLayout()
        
        # Battery Section
        top_dash = QHBoxLayout()
        self.local_bat_frame, self.local_bar = self.create_battery_widget("LOCAL DEVICE (HOST)")
        self.remote_bat_frame, self.remote_bar = self.create_battery_widget("REMOTE TRANSCEIVER (NODE)")
        top_dash.addWidget(self.local_bat_frame)
        top_dash.addWidget(self.remote_bat_frame)
        mid_container.addLayout(top_dash)

        self.chat_area = QTextEdit()
        self.chat_area.setReadOnly(True)
        mid_container.addWidget(self.chat_area, 7)

        self.log_area = QTextEdit()
        self.log_area.setFixedHeight(120)
        mid_container.addWidget(self.log_area, 2)

        # Input Row (Fixed Send Connection)
        input_row = QHBoxLayout()
        self.msg_input = QLineEdit()
        self.msg_input.setPlaceholderText("Broadcast encrypted packet...")
        self.msg_input.returnPressed.connect(self.handle_send) # Enter key to send
        
        self.send_btn = QPushButton("SEND DATA")
        self.send_btn.setFixedSize(120, 40)
        self.send_btn.setStyleSheet(f"background: {ACCENT_GREEN}; color: black; font-weight: bold;")
        self.send_btn.clicked.connect(self.handle_send) 
        
        input_row.addWidget(self.msg_input)
        input_row.addWidget(self.send_btn)
        mid_container.addLayout(input_row)

        main_layout.addLayout(mid_container, 7)

        # 3. RIGHT PANEL
        right_panel = QFrame()
        right_panel.setFixedWidth(300)
        right_panel.setStyleSheet(f"background-color: {SIDEBAR_COLOR}; border-radius: 5px; padding: 5px;")
        right_layout = QVBoxLayout(right_panel)
        
        right_layout.addWidget(QLabel("LINK STRENGTH (RSSI)"))
        self.wifi_bar = self.create_signal_meter(right_layout, "Wi-Fi 2.4GHz")
        self.fso_bar = self.create_signal_meter(right_layout, "Optical Link (FSO)")
        self.lora_bar = self.create_signal_meter(right_layout, "LoRa RF Link")
        
        # Graph
        self.graph_widget = pg.PlotWidget()
        self.graph_widget.setBackground('#000')
        self.graph_widget.setFixedHeight(220)
        self.signal_data = [0]*100
        self.curve = self.graph_widget.plot(self.signal_data, pen=pg.mkPen(color=ACCENT_GREEN, width=2))
        right_layout.addWidget(self.graph_widget)
        
        right_layout.addStretch()
        self.ptt_btn = QPushButton("PUSH TO TALK (PTT)")
        self.ptt_btn.setFixedHeight(50)
        self.ptt_btn.pressed.connect(lambda: self.add_log("PTT: Audio channel open..."))
        right_layout.addWidget(self.ptt_btn)
        main_layout.addWidget(right_panel, 3)

    def create_battery_widget(self, name):
        w = QFrame()
        l = QVBoxLayout(w)
        l.setContentsMargins(0, 0, 0, 0)
        lbl = QLabel(name)
        bar = QProgressBar()
        bar.setAlignment(Qt.AlignmentFlag.AlignCenter)
        l.addWidget(lbl)
        l.addWidget(bar)
        return w, bar

    def create_signal_meter(self, parent_layout, name):
        parent_layout.addWidget(QLabel(f"<font color='#888'>{name}</font>"))
        bar = QProgressBar()
        bar.setAlignment(Qt.AlignmentFlag.AlignCenter)
        bar.setValue(0)
        parent_layout.addWidget(bar)
        return bar

    def handle_send(self):
        txt = self.msg_input.text()
        if txt:
            time = QDateTime.currentDateTime().toString("hh:mm")
            self.chat_area.append(f"<font color='#555'>[{time}]</font> <b style='color:{ACCENT_GREEN}'>YOU:</b> {txt}")
            self.msg_input.clear()
            self.add_log(f"TX: Packet {len(txt)} bytes pushed to transmission buffer.")

    def add_log(self, msg):
        time = QDateTime.currentDateTime().toString("hh:mm:ss")
        self.log_area.append(f"<font color='#00FF41'>></font> {time}: {msg}")

    def update_telemetry(self):
        # 1. Update Signal History Graph
        self.signal_data[:-1] = self.signal_data[1:]
        self.signal_data[-1] = random.randint(50, 95)
        self.curve.setData(self.signal_data)
        
        # 2. LOCAL HARDWARE SENSING (Laptop Battery)
        try:
            battery = psutil.sensors_battery()
            percent = int(battery.percent)
            is_plugged = battery.power_plugged
            status = "CHARGING" if is_plugged else "BATTERY"
            self.local_bar.setValue(percent)
            self.local_bar.setFormat(f"{percent}% ({status})")
            # FIX: Text Visibility Logic
            color = "black" if percent > 45 else ACCENT_GREEN
            self.local_bar.setStyleSheet(f"color: {color};")
        except:
            self.local_bar.setValue(100)
            self.local_bar.setFormat("HW SENSOR ERROR")

        # 3. REMOTE NODE SENSING (Simulating your ESP32)
        if self.remote_charging:
            self.remote_val = min(100, self.remote_val + 1)
        else:
            self.remote_val = max(0, self.remote_val - 1)
            
        self.remote_bar.setValue(self.remote_val)
        mode = "CHARGING" if self.remote_charging else "BATTERY"
        self.remote_bar.setFormat(f"{self.remote_val}% ({mode})")
        
        # FIX: Text Visibility Logic for Remote
        r_color = "black" if self.remote_val > 45 else ACCENT_GREEN
        self.remote_bar.setStyleSheet(f"color: {r_color};")

        # 4. UPDATING METERS (Checking Signal)
        self.wifi_bar.setValue(random.randint(70, 90))
        self.fso_bar.setValue(random.randint(20, 40))
        self.lora_bar.setValue(random.randint(50, 65))

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = SnakeLinkApp()
    window.show()
    sys.exit(app.exec())



# import sys
# import os
# import random
# import pyqtgraph as pg
# from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
#                              QHBoxLayout, QLabel, QPushButton, QTextEdit, 
#                              QLineEdit, QProgressBar, QFrame)
# from PyQt6.QtCore import Qt, QTimer, QDateTime
# from PyQt6.QtGui import QFont

# # --- TACTICAL THEME ---
# BG_COLOR = "#0A0A0B"
# ACCENT_GREEN = "#00FF41"
# SIDEBAR_COLOR = "#121213"

# class SnakeLinkApp(QMainWindow):
#     def __init__(self):
#         super().__init__()
#         self.setWindowTitle("SnakeLink Tactical Dashboard v2.6")
#         self.setMinimumSize(1200, 800)
        
#         # FIXED STYLESHEET: Ensures text is visible and bars are flush
#         self.setStyleSheet(f"""
#             QMainWindow {{ background-color: {BG_COLOR}; }}
#             QLabel {{ color: {ACCENT_GREEN}; font-weight: bold; font-size: 11px; }}
#             QProgressBar {{ 
#                 border: 1px solid #333; 
#                 border-radius: 0px; 
#                 text-align: right; 
#                 background: #050505; 
#                 color: white;
#                 height: 12px;
#                 margin-bottom: 5px;
#             }}
#             QProgressBar::chunk {{ background-color: {ACCENT_GREEN}; width: 1px; }}
#             QTextEdit, QLineEdit {{ 
#                 background-color: #050505; 
#                 border: 1px solid #222; 
#                 color: {ACCENT_GREEN}; 
#                 selection-background-color: #333; 
#             }}
#             QPushButton {{ 
#                 background-color: #1A1A1B; 
#                 border: 1px solid #333; 
#                 color: #888; 
#                 font-weight: bold;
#             }}
#             QPushButton:hover {{ border-color: {ACCENT_GREEN}; color: white; }}
#         """)
        
#         self.initUI()
        
#         self.status_timer = QTimer()
#         self.status_timer.timeout.connect(self.update_telemetry)
#         self.status_timer.start(1000)

#     def initUI(self):
#         central_widget = QWidget()
#         self.setCentralWidget(central_widget)
#         main_layout = QHBoxLayout(central_widget)
#         main_layout.setContentsMargins(5, 5, 5, 5)
#         main_layout.setSpacing(10)

#         # --- 1. SIDEBAR ---
#         sidebar = QFrame()
#         sidebar.setFixedWidth(65)
#         sidebar.setStyleSheet(f"background-color: {SIDEBAR_COLOR}; border-radius: 5px;")
#         sidebar_layout = QVBoxLayout(sidebar)
#         sidebar_layout.setAlignment(Qt.AlignmentFlag.AlignTop)
        
#         for icon in ["💬", "📊", "🛰️", "⚙️"]:
#             btn = QPushButton(icon)
#             btn.setFixedSize(50, 50)
#             btn.setFlat(True)
#             sidebar_layout.addWidget(btn)
#         sidebar_layout.addStretch()
#         main_layout.addWidget(sidebar)

#         # --- 2. CENTER CONTENT ---
#         mid_container = QVBoxLayout()
        
#         # TOP DASHBOARD (Battery Status)
#         top_dash = QHBoxLayout()
#         top_dash.setSpacing(20)
#         self.local_bat, self.local_bar = self.create_battery_widget("LOCAL DEVICE")
#         self.remote_bat, self.remote_bar = self.create_battery_widget("REMOTE TRANSCEIVER")
#         top_dash.addWidget(self.local_bat)
#         top_dash.addWidget(self.remote_bat)
#         mid_container.addLayout(top_dash)

#         self.chat_area = QTextEdit()
#         self.chat_area.setReadOnly(True)
#         mid_container.addWidget(self.chat_area, 7)

#         self.log_area = QTextEdit()
#         self.log_area.setFixedHeight(100)
#         mid_container.addWidget(self.log_area, 2)

#         input_row = QHBoxLayout()
#         self.msg_input = QLineEdit()
#         self.msg_input.setPlaceholderText("Enter secure data packet...")
#         self.send_btn = QPushButton("SEND DATA")
#         self.send_btn.setFixedSize(120, 40)
#         self.send_btn.setStyleSheet(f"background: {ACCENT_GREEN}; color: black;")
#         self.send_btn.clicked.connect(self.handle_send)
#         input_row.addWidget(self.msg_input)
#         input_row.addWidget(self.send_btn)
#         mid_container.addLayout(input_row)

#         main_layout.addLayout(mid_container, 7)

#         # --- 3. RIGHT PANEL (Analytics) ---
#         right_panel = QFrame()
#         right_panel.setFixedWidth(300)
#         right_panel.setStyleSheet(f"background-color: {SIDEBAR_COLOR}; border-radius: 5px;")
#         right_layout = QVBoxLayout(right_panel)
#         right_layout.setAlignment(Qt.AlignmentFlag.AlignTop) # FIX: No more floating in middle
        
#         right_layout.addWidget(QLabel("SYSTEM ANALYTICS"))
#         right_layout.addSpacing(10)
        
#         # Meters
#         self.wifi_bar = self.create_signal_meter(right_layout, "Wi-Fi UDP Link")
#         self.fso_bar = self.create_signal_meter(right_layout, "FSO Optical Link")
#         self.lora_bar = self.create_signal_meter(right_layout, "LoRa Control Channel")
        
#         # GRAPH
#         right_layout.addSpacing(20)
#         right_layout.addWidget(QLabel("SIGNAL HISTORY"))
#         self.graph_widget = pg.PlotWidget()
#         self.graph_widget.setBackground('#000')
#         self.graph_widget.setFixedHeight(220)
#         self.signal_data = [0]*100
#         self.curve = self.graph_widget.plot(self.signal_data, pen=pg.mkPen(color=ACCENT_GREEN, width=2))
#         right_layout.addWidget(self.graph_widget)

#         right_layout.addStretch()
        
#         self.ptt_btn = QPushButton("PUSH TO TALK (PTT)")
#         self.ptt_btn.setFixedHeight(50)
#         right_layout.addWidget(self.ptt_btn)

#         main_layout.addWidget(right_panel, 3)

#     # --- HELPER METHODS ---
#     def create_battery_widget(self, name):
#         w = QFrame()
#         l = QVBoxLayout(w)
#         l.setContentsMargins(0, 0, 0, 0)
#         l.setSpacing(2)
#         lbl = QLabel(name)
#         bar = QProgressBar()
#         bar.setValue(98)
#         bar.setFormat("%p%") # Shows percentage clearly
#         l.addWidget(lbl)
#         l.addWidget(bar)
#         return w, bar

#     def create_signal_meter(self, parent_layout, name):
#         parent_layout.addWidget(QLabel(f"<font color='#888'>{name}</font>"))
#         bar = QProgressBar()
#         bar.setValue(random.randint(40, 90))
#         bar.setFormat("%v %") # Shows raw value + %
#         parent_layout.addWidget(bar)
#         return bar

#     def handle_send(self):
#         txt = self.msg_input.text()
#         if txt:
#             time = QDateTime.currentDateTime().toString("hh:mm")
#             self.chat_area.append(f"<b>[{time}] You:</b> {txt}")
#             self.msg_input.clear()
#             self.add_log(f"Encrypted Packet Transmitted: {len(txt)} bytes")

#     def add_log(self, msg):
#         time = QDateTime.currentDateTime().toString("hh:mm:ss")
#         self.log_area.append(f"> {time}: {msg}")

#     def update_telemetry(self):
#         self.signal_data[:-1] = self.signal_data[1:]
#         self.signal_data[-1] = random.randint(40, 95)
#         self.curve.setData(self.signal_data)
#         # Randomly fluctuate battery for demo
#         if random.random() > 0.8:
#             self.local_bar.setValue(self.local_bar.value() - 1)

# if __name__ == "__main__":
#     app = QApplication(sys.argv)
#     window = SnakeLinkApp()
#     window.show()
#     sys.exit(app.exec())

#===========
# import sys
# import os
# import pyqtgraph as pg  # Industry standard for real-time charts
# from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
#                              QHBoxLayout, QLabel, QPushButton, QTextEdit, 
#                              QLineEdit, QProgressBar, QFrame, QStackedWidget)
# from PyQt6.QtCore import Qt, QTimer, QDateTime
# from PyQt6.QtGui import QFont

# # --- TACTICAL THEME ---
# BG_COLOR = "#0A0A0B"
# ACCENT_GREEN = "#00FF41"
# SIDEBAR_COLOR = "#121213"
# SURFACE_COLOR = "#1E1E1F"

# class SnakeLinkApp(QMainWindow):
#     def __init__(self):  # FIXED: Double underscores
#         super().__init__()
#         self.setWindowTitle("SnakeLink Tactical Dashboard v2.5")
#         self.setMinimumSize(1200, 800)
        
#         # Global Stylesheet for a "Clean" feel
#         self.setStyleSheet(f"""
#             QMainWindow {{ background-color: {BG_COLOR}; }}
#             QLabel {{ color: {ACCENT_GREEN}; }}
#             QPushButton {{ 
#                 background-color: {SIDEBAR_COLOR}; 
#                 border: 1px solid #333; 
#                 border-radius: 4px;
#                 padding: 10px;
#                 color: #888;
#             }}
#             QPushButton:hover {{ border-color: {ACCENT_GREEN}; color: white; }}
#             QProgressBar {{ border: 1px solid #333; border-radius: 2px; text-align: center; background: #000; }}
#             QProgressBar::chunk {{ background-color: {ACCENT_GREEN}; }}
#         """)
        
#         self.initUI()
        
#         # Telemetry Timer
#         self.status_timer = QTimer()
#         self.status_timer.timeout.connect(self.update_telemetry)
#         self.status_timer.start(2000) # Update every 2 seconds for "live" feel

#     def initUI(self):
#         central_widget = QWidget()
#         self.setCentralWidget(central_widget)
#         main_layout = QHBoxLayout(central_widget)
#         main_layout.setContentsMargins(5, 5, 5, 5)
#         main_layout.setSpacing(10)

#         # --- 1. SIDEBAR (STATIONARY) ---
#         sidebar = QFrame()
#         sidebar.setFixedWidth(70)
#         sidebar.setStyleSheet(f"background-color: {SIDEBAR_COLOR}; border-radius: 10px;")
#         sidebar_layout = QVBoxLayout(sidebar)
        
#         self.nav_btns = []
#         icons = [("💬", "Chat"), ("📊", "Data"), ("🛰️", "GPS"), ("⚙️", "Settings")]
#         for icon, name in icons:
#             btn = QPushButton(icon)
#             btn.setToolTip(name)
#             btn.setFixedSize(55, 55)
#             btn.clicked.connect(lambda _, n=name: self.add_log(f"Switched to {n} view"))
#             sidebar_layout.addWidget(btn)
#             self.nav_btns.append(btn)
        
#         sidebar_layout.addStretch()
#         main_layout.addWidget(sidebar)

#         # --- 2. CENTER: CHAT & LOGS ---
#         mid_container = QVBoxLayout()
        
#         # Battery Section
#         top_dash = QHBoxLayout()
#         self.local_bat = self.create_battery_widget("LOCAL DEVICE")
#         self.remote_bat = self.create_battery_widget("REMOTE TRANSCEIVER")
#         top_dash.addWidget(self.local_bat)
#         top_dash.addWidget(self.remote_bat)
#         mid_container.addLayout(top_dash)

#         self.chat_area = QTextEdit()
#         self.chat_area.setReadOnly(True)
#         self.chat_area.setStyleSheet("background-color: #050505; border: 1px solid #222; border-radius: 8px; padding: 10px;")
#         mid_container.addWidget(self.chat_area, 7)

#         self.log_area = QTextEdit()
#         self.log_area.setFixedHeight(120)
#         self.log_area.setReadOnly(True)
#         self.log_area.setStyleSheet("background-color: #000; color: #555; font-size: 11px; border: 1px solid #222; border-radius: 8px;")
#         mid_container.addWidget(self.log_area, 2)

#         # Input Area
#         input_row = QHBoxLayout()
#         self.msg_input = QLineEdit()
#         self.msg_input.setPlaceholderText("Type secure message...")
#         self.msg_input.setStyleSheet("background: #111; padding: 12px; border: 1px solid #333; border-radius: 5px;")
        
#         self.send_btn = QPushButton("SEND DATA")
#         self.send_btn.setFixedWidth(120)
#         self.send_btn.setStyleSheet(f"background: {ACCENT_GREEN}; color: black; font-weight: bold;")
#         self.send_btn.clicked.connect(self.handle_send) # BUTTON NOW WORKS
        
#         input_row.addWidget(self.msg_input)
#         input_row.addWidget(self.send_btn)
#         mid_container.addLayout(input_row)

#         main_layout.addLayout(mid_container, 6)

#         # --- 3. RIGHT PANEL: METERS & LIVE CHART ---
#         right_panel = QFrame()
#         right_panel.setFixedWidth(320)
#         right_panel.setStyleSheet(f"background-color: {SIDEBAR_COLOR}; border-radius: 10px; padding: 10px;")
#         right_layout = QVBoxLayout(right_panel)
        
#         right_layout.addWidget(QLabel("<b>LINK ANALYTICS</b>"))
        
#         self.wifi_m = self.create_signal_meter("Wi-Fi MESH", 80)
#         self.fso_m = self.create_signal_meter("FSO LASER", 40)
#         self.lora_m = self.create_signal_meter("LoRa BACKHAUL", 60)
        
#         right_layout.addLayout(self.wifi_m)
#         right_layout.addLayout(self.fso_m)
#         right_layout.addLayout(self.lora_m)

#         # PROFESSIONAL CHART (Using PyQtGraph)
#         right_layout.addSpacing(20)
#         right_layout.addWidget(QLabel("<b>SIGNAL STRENGTH HISTORY</b>"))
#         self.graph_widget = pg.PlotWidget()
#         self.graph_widget.setBackground('#000')
#         self.graph_widget.setLabel('left', 'RSSI', units='dB')
#         self.graph_widget.showGrid(x=True, y=True)
#         self.signal_data = [0]*50
#         self.curve = self.graph_widget.plot(self.signal_data, pen=pg.mkPen(color=ACCENT_GREEN, width=2))
#         right_layout.addWidget(self.graph_widget)

#         right_layout.addStretch()
        
#         self.ptt_btn = QPushButton("PUSH TO TALK (PTT)")
#         self.ptt_btn.setFixedHeight(50)
#         self.ptt_btn.pressed.connect(lambda: self.ptt_btn.setStyleSheet("background: #FF3131; color: white;"))
#         self.ptt_btn.released.connect(lambda: self.ptt_btn.setStyleSheet(""))
#         right_layout.addWidget(self.ptt_btn)

#         main_layout.addWidget(right_panel, 3)

#     # --- HELPER METHODS ---
#     def create_battery_widget(self, name):
#         w = QFrame()
#         l = QVBoxLayout(w)
#         l.addWidget(QLabel(f"<small>{name}</small>"))
#         bar = QProgressBar()
#         bar.setValue(95)
#         bar.setFixedHeight(10)
#         l.addWidget(bar)
#         return w

#     def create_signal_meter(self, name, val):
#         l = QVBoxLayout()
#         l.addWidget(QLabel(name))
#         bar = QProgressBar()
#         bar.setValue(val)
#         bar.setFixedHeight(8)
#         l.addWidget(bar)
#         return l

#     def handle_send(self):
#         txt = self.msg_input.text()
#         if txt:
#             time = QDateTime.currentDateTime().toString("hh:mm")
#             self.chat_area.append(f"<font color='#888'>[{time}]</font> <b>You:</b> {txt}")
#             self.msg_input.clear()
#             self.add_log("Data Packet Encrypted and Transmitted via FSO")

#     def add_log(self, msg):
#         time = QDateTime.currentDateTime().toString("hh:mm:ss")
#         self.log_area.append(f"<b>></b> {time}: {msg}")

#     def update_telemetry(self):
#         # Update graph data
#         import random
#         self.signal_data[:-1] = self.signal_data[1:]
#         self.signal_data[-1] = random.randint(30, 90)
#         self.curve.setData(self.signal_data)

# if __name__ == "__main__":
#     app = QApplication(sys.argv)
#     window = SnakeLinkApp()
#     window.show()
#     sys.exit(app.exec())