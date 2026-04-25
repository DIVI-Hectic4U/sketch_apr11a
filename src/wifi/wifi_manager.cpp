#include "wifi_manager.h"
#include "../state/app_state.h"

void WiFiManagerWrapper::init() {
    Serial.println("WiFi: Initializing in station mode...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(); // Attempt to connect to saved credentials automatically
}

void WiFiManagerWrapper::scanNetworks() {
    Serial.println("WiFi: Preparing scan...");
    
    // Ensure we are in station mode and disconnected to avoid conflicts
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.println("WiFi: Scanning...");
    int n = WiFi.scanNetworks(false, true, false, 150); // Async=false, show_hidden=true, passive=false, time_per_chan=150ms
    
    AppState& state = AppState::getInstance();
    state.scannedSSIDs.clear();
    
    if (n == 0) {
        Serial.println("WiFi: No networks found.");
    } else {
        Serial.printf("WiFi: %d networks found.\n", n);
        for (int i = 0; i < n; ++i) {
            state.scannedSSIDs.push_back(WiFi.SSID(i));
            delay(10);
        }
    }
    WiFi.scanDelete();
}

void WiFiManagerWrapper::connectTo(String ssid, String password) {
    Serial.printf("WiFi: Connecting to %s...\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());
}

void WiFiManagerWrapper::update() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 5000) {
        lastCheck = millis();
        bool connected = (WiFi.status() == WL_CONNECTED);
        
        AppState& state = AppState::getInstance();
        if (state.isWifiConnected != connected) {
            state.isWifiConnected = connected;
            if (connected) {
                state.localIP = WiFi.localIP().toString();
                state.currentSSID = WiFi.SSID();
                Serial.printf("WiFi: Connected! IP: %s\n", state.localIP.c_str());
            } else {
                state.localIP = "0.0.0.0";
                state.currentSSID = "Disconnected";
                Serial.println("WiFi: Connection lost.");
            }
        }
    }
}
