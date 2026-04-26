"""
SnakeLink Software Configuration
"""

import json
import os
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import Optional


@dataclass
class AppConfig:
    """Application configuration with persistence."""

    # Device
    device_name: str = "SnakeLink-Laptop"
    transceiver_ip: str = "192.168.4.1"
    transceiver_port: int = 4210
    local_port: int = 4210
    paired_device_id: str = ""

    # Security
    encryption_key: str = ""  # Base64 encoded 32-byte key
    high_security_mode: bool = False
    auto_wipe_on_exit: bool = False

    # Audio
    mic_device: int = 0
    speaker_device: int = 0
    ptt_key: str = "Space"
    alert_volume: int = 80

    # File Transfer
    auto_compress: bool = True
    compression_quality: int = 60
    save_directory: str = str(Path.home() / "SnakeLink" / "Downloads")

    # Link Thresholds (FSO-First Architecture)
    # FSO = PRIMARY (laser), Wi-Fi = BACKUP, LoRa = EMERGENCY
    fso_voltage_degrade: float = 1.0   # Switch from FSO to Wi-Fi below this
    fso_voltage_recover: float = 1.5   # Return to FSO above this
    wifi_rssi_degrade: int = -80       # Switch from Wi-Fi to LoRa below this
    wifi_rssi_recover: int = -75       # Return to Wi-Fi above this
    stability_window_ms: int = 5000

    # Map
    map_tile_dir: str = str(Path.home() / "SnakeLink" / "MapTiles")
    default_lat: float = 7.8731  # Sri Lanka center
    default_lon: float = 80.7718

    # UI
    dark_theme: bool = True
    show_delivery_ticks: bool = True

    def __post_init__(self):
        self._config_dir = Path.home() / ".snakelink"
        self._config_file = self._config_dir / "config.json"
        self._ensure_dirs()

    def _ensure_dirs(self):
        self._config_dir.mkdir(parents=True, exist_ok=True)
        Path(self.save_directory).mkdir(parents=True, exist_ok=True)
        Path(self.map_tile_dir).mkdir(parents=True, exist_ok=True)

    def load(self) -> bool:
        """Load configuration from file."""
        try:
            if self._config_file.exists():
                with open(self._config_file, 'r') as f:
                    data = json.load(f)
                for key, value in data.items():
                    if hasattr(self, key):
                        setattr(self, key, value)
                return True
        except Exception as e:
            print(f"[Config] Load error: {e}")
        return False

    def save(self) -> bool:
        """Save configuration to file."""
        try:
            with open(self._config_file, 'w') as f:
                json.dump(asdict(self), f, indent=2)
            return True
        except Exception as e:
            print(f"[Config] Save error: {e}")
        return False

    def get_key_bytes(self) -> Optional[bytes]:
        """Get encryption key as bytes."""
        import base64
        if not self.encryption_key:
            return None
        try:
            return base64.b64decode(self.encryption_key)
        except:
            return None

    def set_key_bytes(self, key: bytes):
        """Set encryption key from bytes."""
        import base64
        self.encryption_key = base64.b64encode(key).decode('ascii')
