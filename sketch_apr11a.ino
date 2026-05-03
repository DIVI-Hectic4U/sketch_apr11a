#include "display.h"
#include "pin_verify.h"
#include "src/state/app_state.h"
#include "src/ui/ui_manager.h"
#include "src/wifi/wifi_manager.h"
#include "src/state/session_machine.h"
#include "src/api/api_client.h"

// --- Hardware Bridge Pins ---
#define ARDUINO_TX_PIN 4

void setup() {
    // 1. Serial Initialization
    Serial.begin(9600);
    delay(2000); 
    
    // 2. Hardware Pin Verification
    printPinVerification();

    // 3. Display & Graphics Driver Init
    display_init();

    // 4. Initial Screen (Boot)
    UIManager::getInstance().moveTo(Screen::BOOT);
    lv_timer_handler(); 
    Serial.println("System Booting...");

    // 5. WiFi Initialization
    WiFiManagerWrapper::getInstance().init();
    APIClient::getInstance().init();

    // 6. Transition to Home after a short delay
    delay(2000); 
    UIManager::getInstance().moveTo(Screen::HOME);
    
    Serial.println("Setup Complete!");
}

// Timing trackers
static unsigned long lastTick = 0;
static unsigned long lastWifiUpdate = 0;
static unsigned long lastSessionTick = 0;
static unsigned long lastWatchdog = 0;

void loop() {
    unsigned long now = millis();

    // --- LVGL TICK: Tell LVGL how much time has passed ---
    if (lastTick == 0) lastTick = now;
    unsigned long elapsed = now - lastTick;
    if (elapsed > 0) {
        lv_tick_inc(elapsed);
        lastTick = now;
    }

    // --- LVGL HANDLER: Process UI events and animations ---
    lv_timer_handler();

    // --- API: Process WebSocket messages ---
    APIClient::getInstance().update();

    // --- Check for UI rebuild signaling ---
    AppState& state = AppState::getInstance();
    if (state.dashboardDirty) {
        state.dashboardDirty = false;
        Serial.println("[UI] Dashboard dirty, refreshing current screen...");
        UIManager::getInstance().refreshCurrentScreen();
    }

    // --- Session Machine: tick every 100ms ---
    if (now - lastSessionTick >= 100) {
        lastSessionTick = now;
        SessionMachine::getInstance().tick();
    }

    // --- WiFi Monitor: check every 1 second ---
    if (now - lastWifiUpdate >= 1000) {
        lastWifiUpdate = now;
        WiFiManagerWrapper::getInstance().update();
    }

    // --- Watchdog: print heap every 10 seconds ---
    if (now - lastWatchdog >= 10000) {
        lastWatchdog = now;
        Serial.printf("[Watchdog] Free Heap: %d bytes | Min: %d bytes\n",
                      ESP.getFreeHeap(), ESP.getMinFreeHeap());
    }

    // A tiny delay to keep the ESP32 system tasks happy
    delay(1);
}
