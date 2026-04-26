"""
AES-256-GCM Encryption Manager (Python)
"""

from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from typing import Optional


class CryptoManager:
    """AES-256-GCM encryption/decryption."""

    def __init__(self):
        self._key: Optional[bytes] = None
        self._cipher: Optional[AESGCM] = None

    def set_key(self, key: bytes) -> bool:
        """Set 32-byte encryption key."""
        if len(key) != 32:
            return False
        self._key = key
        self._cipher = AESGCM(key)
        return True

    def is_ready(self) -> bool:
        return self._cipher is not None

    def encrypt(self, plaintext: bytes, aad: bytes = b"") -> Optional[bytes]:
        """Encrypt plaintext. Returns IV(12) + ciphertext + tag(16)."""
        if not self._cipher:
            return None
        try:
            import os
            iv = os.urandom(12)
            ciphertext = self._cipher.encrypt(iv, plaintext, aad)
            return iv + ciphertext
        except Exception as e:
            print(f"[Crypto] Encrypt error: {e}")
            return None

    def decrypt(self, ciphertext: bytes, aad: bytes = b"") -> Optional[bytes]:
        """Decrypt ciphertext. Input: IV(12) + ciphertext + tag(16)."""
        if not self._cipher or len(ciphertext) < 28:
            return None
        try:
            iv = ciphertext[:12]
            ct = ciphertext[12:]
            return self._cipher.decrypt(iv, ct, aad)
        except Exception as e:
            print(f"[Crypto] Decrypt error: {e}")
            return None
