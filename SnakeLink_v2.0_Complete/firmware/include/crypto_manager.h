/**
 * @file crypto_manager.h
 * @brief AES-256-GCM Encryption/Decryption
 * @description Hardware-accelerated via ESP32 mbedTLS.
 */

#ifndef CRYPTO_MANAGER_H
#define CRYPTO_MANAGER_H

#include "config.h"
#include <mbedtls/gcm.h>

namespace crypto {

class CryptoManager {
public:
    CryptoManager();
    ~CryptoManager();

    bool begin();
    void end();

    // Set key (32 bytes)
    bool setKey(const uint8_t* key, size_t len);

    // Encrypt: plaintext -> ciphertext + auth tag
    // Output: iv (12B) + ciphertext + tag (16B)
    // Returns total output length, or -1 on error
    int encrypt(const uint8_t* plaintext, size_t ptLen,
                uint8_t* output, size_t outLen,
                const uint8_t* aad = nullptr, size_t aadLen = 0);

    // Decrypt: iv + ciphertext + tag -> plaintext
    // Returns plaintext length, or -1 on error
    int decrypt(const uint8_t* input, size_t inLen,
                uint8_t* plaintext, size_t ptMaxLen,
                const uint8_t* aad = nullptr, size_t aadLen = 0);

    // Generate random IV
    void generateIV(uint8_t* iv, size_t len = 12);

    // Key derivation (simple XOR-based for pre-shared, or HKDF in future)
    bool deriveKey(const uint8_t* sharedSecret, size_t len, uint8_t* outKey);

    bool isReady() const { return keySet; }

private:
    mbedtls_gcm_context gcm;
    uint8_t key[DEVICE_KEY_LEN];
    bool keySet;
    bool initialized;
};

} // namespace crypto

#endif // CRYPTO_MANAGER_H
