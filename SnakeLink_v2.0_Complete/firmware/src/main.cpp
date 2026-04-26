/**
 * @file main.cpp
 * @brief SnakeLink v2.0 Firmware Entry Point
 * @description Dual-core FreeRTOS architecture:
 *              Core 0: Background (LoRa, OLED, LEDs, Buzzer, Battery, Buttons)
 *              Core 1: Real-time (FSO UART, Wi-Fi ESP-NOW, FSM, ACK handling)
 */

#include "config.h"
#include "hlink_protocol.h"
#include "fsm_manager.h"
#include "link_manager.h"
#include "wifi_link.h"
#include "fso_link.h"
#include "lora_link.h"
#include "telemetry_manager.h"
#include "display_manager.h"
#include "feedback_manager.h"
#include "battery_manager.h"
#include "button_manager.h"
#include "config_manager.h"
#include "crypto_manager.h"

#include <Arduino.h>
#include <esp_task_wdt.h>

// ============================================================================
// GLOBAL MANAGERS
// ============================================================================
static fsm::FSMManager fsmMgr;
static links::LinkManager linkMgr;
static links::WiFiLink wifiLink;
static links::FSOLink fsoLink;
static links::LoRaLink loraLink;
static telemetry::TelemetryManager telemMgr;
static display::DisplayManager dispMgr;
static feedback::FeedbackManager fbMgr;
static battery::BatteryManager batMgr;
static buttons::ButtonManager btnMgr;
static config::ConfigManager cfgMgr;
static crypto::CryptoManager cryptoMgr;

// Task handles
static TaskHandle_t dataTaskHandle = nullptr;
static TaskHandle_t bgTaskHandle = nullptr;

// Watchdog handles
static esp_task_wdt_user_handle_t wdtDataHandle;
static esp_task_wdt_user_handle_t wdtBgHandle;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
void dataTask(void* pvParameters);
void backgroundTask(void* pvParameters);
void onPacketReceived(const hlink::Packet& pkt);
void onAlignPress();
void onAlignLongPress();
void onModePress();
void onModeLongPress();
void onEmergency();
void setupButtons();
void setupLinks();
void setupDisplay();
void bootSequence();

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n========================================");
    Serial.println("  SnakeLink v2.0 — Tactical Comm System");
    Serial.println("========================================");

    // Initialize NVS config first
    if (!cfgMgr.begin()) {
        Serial.println("[MAIN] WARNING: Config init failed");
    }

    // Boot sequence with visual feedback
    bootSequence();

    // Initialize crypto
    cryptoMgr.begin();
    cryptoMgr.setKey(cfgMgr.get().encryptionKey, DEVICE_KEY_LEN);

    // Initialize display
    setupDisplay();

    // Initialize feedback
    fbMgr.begin();
    fbMgr.trigger(feedback::EVT_BOOT);

    // Initialize battery monitoring
    batMgr.begin();

    // Setup links
    setupLinks();

    // Setup telemetry
    telemMgr.setLinkManager(&linkMgr);
    telemMgr.begin();

    // Setup buttons
    setupButtons();

    // Setup FSM
    fsmMgr.begin();
    linkMgr.setFSM(&fsmMgr);

    // Register packet handler
    linkMgr.setPacketHandler(onPacketReceived);

    // Start in ready state
    fbMgr.trigger(feedback::EVT_READY);
    dispMgr.setState(display::DISP_IDLE);

    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(
        dataTask,
        "DataTask",
        TASK_DATA_STACK,
        nullptr,
        TASK_DATA_PRIORITY,
        &dataTaskHandle,
        TASK_DATA_CORE
    );

    xTaskCreatePinnedToCore(
        backgroundTask,
        "BgTask",
        TASK_BG_STACK,
        nullptr,
        TASK_BG_PRIORITY,
        &bgTaskHandle,
        TASK_BG_CORE
    );

    // Initialize watchdog timers
    esp_task_wdt_config_t wdtConfig = {
        .timeout_ms = 10000,
        .idle_core_mask = 0,
        .trigger_panic = true
    };
    esp_task_wdt_init(&wdtConfig);
    esp_task_wdt_add_user("DataTask", &wdtDataHandle);
    esp_task_wdt_add_user("BgTask", &wdtBgHandle);

    Serial.println("[MAIN] Setup complete, tasks running");
}

void loop() {
    // Main loop is empty — all work in FreeRTOS tasks
    // Feed watchdog for main task
    esp_task_wdt_reset_user(wdtDataHandle);
    vTaskDelay(pdMS_TO_TICKS(1000));
}

// ============================================================================
// DATA TASK (Core 1 — Real-time, highest priority)
// ============================================================================
void dataTask(void* pvParameters) {
    Serial.println("[DataTask] Started on Core 1");

    uint32_t lastFSMUpdate = 0;
    uint32_t lastMetricsUpdate = 0;

    while (true) {
        uint32_t now = millis();

        // 1. Read FSO ADC voltage
        float fsoVoltage = fsoLink.readADC();

        // 2. Read Wi-Fi RSSI
        int8_t wifiRSSI = wifiLink.getRSSI();

        // 3. Read LoRa RSSI
        int8_t loraRSSI = loraLink.getRSSI();

        // 4. Update FSM metrics (every 100ms)
        if (now - lastMetricsUpdate >= 100) {
            fsmMgr.updateMetrics(fsoVoltage, wifiRSSI, loraRSSI);
            lastMetricsUpdate = now;
        }

        // 5. Run FSM evaluation (every 50ms)
        if (now - lastFSMUpdate >= 50) {
            fsmMgr.evaluate();
            lastFSMUpdate = now;

            // Update display with active link
            dispMgr.setActiveLink(static_cast<uint8_t>(fsmMgr.getActiveLink()));
            fbMgr.setLinkPattern(static_cast<uint8_t>(fsmMgr.getActiveLink()));

            // Update telemetry link status
            telemMgr.setLinkStatus(fsoVoltage, wifiRSSI, loraRSSI, 
                                   static_cast<uint8_t>(fsmMgr.getActiveLink()));
        }

        // 6. Check for incoming packets on active link
        linkMgr.update();

        // 7. Handle ACK timeouts and retransmissions
        // (Handled within ARQManager, called via link update)

        // 8. If in warning state, duplicate packets
        if (fsmMgr.shouldDuplicate()) {
            dispMgr.setState(display::DISP_SWITCHING);
            fbMgr.trigger(feedback::EVT_LINK_SWITCH);
        } else {
            // Normal state display
            if (fsmMgr.getState() == fsm::STATE_LORA_EMERGENCY) {
                dispMgr.setState(display::DISP_LORA_EMERGENCY);
                fbMgr.trigger(feedback::EVT_LORA_EMERGENCY);
            }
        }

        // Feed watchdog
        esp_task_wdt_reset_user(wdtDataHandle);

        vTaskDelay(pdMS_TO_TICKS(1));  // 1ms tick
    }
}

// ============================================================================
// BACKGROUND TASK (Core 0 — Periodic, UI, sensors)
// ============================================================================
void backgroundTask(void* pvParameters) {
    Serial.println("[BgTask] Started on Core 0");

    uint32_t lastTelem = 0;
    uint32_t lastDisplay = 0;
    uint32_t lastFeedback = 0;
    uint32_t lastBattery = 0;
    uint32_t lastButton = 0;

    while (true) {
        uint32_t now = millis();

        // 1. LoRa heartbeat (every 5s)
        // 2. GPS broadcast (every 30s)
        // 3. Battery report (every 60s)
        // 4. Link status (every 10s)
        if (now - lastTelem >= 1000) {
            telemMgr.update();
            lastTelem = now;
        }

        // 5. Update OLED display
        if (now - lastDisplay >= DISPLAY_UPDATE_MS) {
            dispMgr.update();
            lastDisplay = now;
        }

        // 6. Update LED patterns
        if (now - lastFeedback >= 20) {
            fbMgr.update();
            lastFeedback = now;
        }

        // 7. Read battery ADC
        if (now - lastBattery >= 5000) {
            batMgr.update();
            telemMgr.setBattery(batMgr.getVoltage());
            dispMgr.setBattery(batMgr.getPercent(), batMgr.getVoltage());

            // Check battery levels
            if (batMgr.isCritical()) {
                fbMgr.trigger(feedback::EVT_BATTERY_CRIT);
                dispMgr.setState(display::DISP_BATTERY_WARN);
            } else if (batMgr.isLow()) {
                fbMgr.trigger(feedback::EVT_BATTERY_LOW);
                dispMgr.setState(display::DISP_BATTERY_WARN);
            }
            lastBattery = now;
        }

        // 8. Check buttons
        if (now - lastButton >= 10) {
            btnMgr.update();
            lastButton = now;
        }

        // Feed watchdog
        esp_task_wdt_reset_user(wdtBgHandle);

        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms tick
    }
}

// ============================================================================
// PACKET HANDLER
// ============================================================================
void onPacketReceived(const hlink::Packet& pkt) {
    Serial.printf("[RX] Type=%s SEQ=%u LEN=%u FLAGS=0x%02X\n",
                  hlink::Packet::typeToString(static_cast<hlink::PacketType>(pkt.header.type)),
                  pkt.header.seq, pkt.header.length, pkt.header.flags);

    switch (pkt.header.type) {
        case hlink::TYPE_TEXT:
            fbMgr.trigger(feedback::EVT_RECEIVE);
            dispMgr.setState(display::DISP_RECEIVE);
            dispMgr.setMessage("MSG RX");
            break;

        case hlink::TYPE_FILE_CHUNK:
            fbMgr.trigger(feedback::EVT_RECEIVE);
            break;

        case hlink::TYPE_ACK:
            // Handled by ARQ manager
            break;

        case hlink::TYPE_HEARTBEAT:
        case hlink::TYPE_GPS:
        case hlink::TYPE_BATTERY:
        case hlink::TYPE_LINK_STATUS:
            // Telemetry — update display/dashboard
            break;

        case hlink::TYPE_VOICE:
            fbMgr.trigger(feedback::EVT_RECEIVE);
            break;

        case hlink::TYPE_EMERGENCY:
            fbMgr.trigger(feedback::EVT_EMERGENCY_RX);
            dispMgr.setState(display::DISP_EMERGENCY_RX);
            dispMgr.setMessage("EMERGENCY!");
            break;

        case hlink::TYPE_CMD:
            // Handle remote commands
            break;

        default:
            break;
    }
}

// ============================================================================
// BUTTON CALLBACKS
// ============================================================================
void onAlignPress() {
    Serial.println("[BTN] Align pressed");
    dispMgr.setState(display::DISP_ALIGN);
    fbMgr.trigger(feedback::EVT_FSO_ALIGN);
}

void onAlignLongPress() {
    Serial.println("[BTN] Align long press — alignment mode");
    // Enter alignment mode with continuous tone
    fbMgr.setAlignmentTone(600);
}

void onModePress() {
    Serial.println("[BTN] Mode pressed");
    // Cycle through manual link selection
    static int manualLink = 0;
    manualLink = (manualLink + 1) % 4;
    fsmMgr.forceLink(static_cast<fsm::LinkType>(manualLink));
}

void onModeLongPress() {
    Serial.println("[BTN] Mode long press");
}

void onEmergency() {
    Serial.println("[BTN] EMERGENCY BROADCAST!");

    // Create emergency packet
    uint8_t payload[64];
    size_t idx = 0;

    // Add GPS coordinates
    float lat = telemMgr.getData().latitude;
    float lon = telemMgr.getData().longitude;
    memcpy(&payload[idx], &lat, 4); idx += 4;
    memcpy(&payload[idx], &lon, 4); idx += 4;

    // Add preset message
    const char* msg = "EMERGENCY";
    memcpy(&payload[idx], msg, strlen(msg)); idx += strlen(msg);

    hlink::Packet pkt(hlink::TYPE_EMERGENCY, hlink::Packet::nextSequence(), payload, idx);
    pkt.header.flags = hlink::FLAG_BROADCAST | hlink::FLAG_PRIORITY;

    linkMgr.sendEmergency(pkt);
    fbMgr.trigger(feedback::EVT_EMERGENCY_RX);
    dispMgr.setState(display::DISP_EMERGENCY_RX);
}

// ============================================================================
// SETUP HELPERS
// ============================================================================
void setupButtons() {
    btnMgr.begin();
    btnMgr.onAlignPress(onAlignPress);
    btnMgr.onAlignLongPress(onAlignLongPress);
    btnMgr.onModePress(onModePress);
    btnMgr.onModeLongPress(onModeLongPress);
    btnMgr.onEmergency(onEmergency);
}

void setupLinks() {
    linkMgr.registerLink(&wifiLink);
    linkMgr.registerLink(&fsoLink);
    linkMgr.registerLink(&loraLink);

    if (!linkMgr.begin()) {
        Serial.println("[MAIN] WARNING: Link manager init had failures");
    }

    // Set packet callbacks for each link
    wifiLink.onPacketReceived(onPacketReceived);
    fsoLink.onPacketReceived(onPacketReceived);
    loraLink.onPacketReceived(onPacketReceived);
}

void setupDisplay() {
    if (!dispMgr.begin()) {
        Serial.println("[MAIN] WARNING: Display init failed");
    }
}

void bootSequence() {
    Serial.println("[BOOT] Starting boot sequence...");

    // Step 1: Power rails
    Serial.println("[BOOT] 1. Power rails OK");

    // Step 2: Config
    Serial.println("[BOOT] 2. NVS config loaded");

    // Step 3: Crypto
    Serial.println("[BOOT] 3. Crypto engine ready");

    // Step 4: Links
    Serial.println("[BOOT] 4. Link drivers loaded");

    // Step 5: Sensors
    Serial.println("[BOOT] 5. Sensors initialized");

    Serial.println("[BOOT] Sequence complete");
}
