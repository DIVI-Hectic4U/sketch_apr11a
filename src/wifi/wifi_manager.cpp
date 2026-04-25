#include "wifi_manager.h"
#include <WiFiManager.h>
#include "../state/app_state.h"

void WiFiManagerWrapper::init() {
    // WiFiManager is blocking by default, but we can configure it.
    // For now, we'll use it in a way that allows us to see the setup portal if needed.
    Serial.println("Initializing WiFi...");
}

void WiFiManagerWrapper::update() {
    if (millis() - lastCheck > 5000) {
        lastCheck = millis();
        bool connected = (WiFi.status() == WL_CONNECTED);
        
        // Update global state
        AppState& state = AppState::getInstance();
        state.isWifiConnected = connected;
        if (connected) {
            state.localIP = WiFi.localIP().toString();
        }
    }
}
