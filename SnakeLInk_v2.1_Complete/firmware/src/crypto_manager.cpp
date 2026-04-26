/**
 * @file crypto_manager.cpp
 */

#include "crypto_manager.h"
#include <Arduino.h>

namespace crypto {

CryptoManager::CryptoManager() : keySet(false), initialized(false) {
    memset(key, 0, sizeof(key));
}

CryptoManager::~CryptoManager() {
    end();
}

bool CryptoManager::begin() {
    mbedtls_gcm_init(&gcm);
    initialized = true;
    return true;
}

void CryptoManager::end() {
    mbedtls_gcm_free(&gcm);
    initialized = false;
    keySet = false;
}

bool CryptoManager::setKey(const uint8_t* k, size_t len) {
    if (!initialized || len != DEVICE_KEY_LEN) return false;

    memcpy(key, k, DEVICE_KEY_LEN);

    int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256);
    if (ret != 0) {
        Serial.printf("[Crypto] GCM setkey failed: %d\n", ret);
        return false;
    }

    keySet = true;
    Serial.println("[Crypto] Key set (256-bit)");
    return true;
}

void CryptoManager::generateIV(uint8_t* iv, size_t len) {
    esp_fill_random(iv, len);
}

int CryptoManager::encrypt(const uint8_t* plaintext, size_t ptLen,
                           uint8_t* output, size_t outLen,
                           const uint8_t* aad, size_t aadLen) {
    if (!keySet || ptLen == 0) return -1;

    const size_t IV_LEN = 12;
    const size_t TAG_LEN = 16;

    if (outLen < IV_LEN + ptLen + TAG_LEN) return -1;

    // Generate random IV
    uint8_t iv[IV_LEN];
    generateIV(iv, IV_LEN);

    // Copy IV to output
    memcpy(output, iv, IV_LEN);

    // Encrypt
    uint8_t tag[TAG_LEN];
    int ret = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT,
                                        ptLen, iv, IV_LEN,
                                        aad, aadLen,
                                        plaintext, output + IV_LEN,
                                        TAG_LEN, tag);
    if (ret != 0) {
        Serial.printf("[Crypto] Encrypt failed: %d\n", ret);
        return -1;
    }

    // Append tag
    memcpy(output + IV_LEN + ptLen, tag, TAG_LEN);

    return IV_LEN + ptLen + TAG_LEN;
}

int CryptoManager::decrypt(const uint8_t* input, size_t inLen,
                           uint8_t* plaintext, size_t ptMaxLen,
                           const uint8_t* aad, size_t aadLen) {
    if (!keySet || inLen < 28) return -1;  // IV(12) + min payload(1) + tag(16) = 29? Actually 12+1+16=29

    const size_t IV_LEN = 12;
    const size_t TAG_LEN = 16;

    if (inLen < IV_LEN + TAG_LEN + 1) return -1;

    size_t ctLen = inLen - IV_LEN - TAG_LEN;
    if (ctLen > ptMaxLen) return -1;

    const uint8_t* iv = input;
    const uint8_t* ciphertext = input + IV_LEN;
    const uint8_t* tag = input + IV_LEN + ctLen;

    int ret = mbedtls_gcm_auth_decrypt(&gcm, ctLen,
                                       iv, IV_LEN,
                                       aad, aadLen,
                                       tag, TAG_LEN,
                                       ciphertext, plaintext);
    if (ret != 0) {
        Serial.printf("[Crypto] Decrypt/auth failed: %d\n", ret);
        return -1;
    }

    return ctLen;
}

bool CryptoManager::deriveKey(const uint8_t* sharedSecret, size_t len, uint8_t* outKey) {
    if (len < DEVICE_KEY_LEN) return false;
    // Simple: use first 32 bytes of shared secret
    // In production, use HKDF-SHA256
    memcpy(outKey, sharedSecret, DEVICE_KEY_LEN);
    return true;
}

} // namespace crypto
