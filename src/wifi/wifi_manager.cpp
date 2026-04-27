#include "wifi_manager.h"
#include "../state/app_state.h"
#include <Preferences.h>

void WiFiManagerWrapper::init() {
    Serial.println("WiFi: Initializing in station mode...");
    WiFi.mode(WIFI_STA);
    
    Preferences prefs;
    prefs.begin("wifi", true);
    String ssid = prefs.getString("ssid", "");
    String pass = prefs.getString("pass", "");
    prefs.end();

    if (ssid.length() > 0) {
        Serial.printf("WiFi: Auto-connecting to saved network: %s\n", ssid.c_str());
        WiFi.begin(ssid.c_str(), pass.c_str());
    } else {
        Serial.println("WiFi: No saved credentials.");
    }
}

void WiFiManagerWrapper::scanNetworksAsync() {
    if (_isScanning) return;
    Serial.println("WiFi: Starting async scan...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    _isScanning = true;
    WiFi.scanNetworks(true); // async = true
}

void WiFiManagerWrapper::connectTo(String ssid, String password) {
    Serial.printf("WiFi: Connecting to %s...\n", ssid.c_str());
    
    Preferences prefs;
    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", password);
    prefs.end();

    WiFi.begin(ssid.c_str(), password.c_str());
}

void WiFiManagerWrapper::update() {
    static unsigned long lastCheck = 0;
    
    // Check scan results
    if (_isScanning) {
        int n = WiFi.scanComplete();
        if (n >= 0) {
            AppState& state = AppState::getInstance();
            state.scannedNetworks.clear();
            Serial.printf("WiFi: Async scan complete. %d networks found.\n", n);
            for (int i = 0; i < n; ++i) {
                ScannedNetwork net;
                net.ssid = WiFi.SSID(i);
                net.rssi = WiFi.RSSI(i);
                net.isOpen = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
                state.scannedNetworks.push_back(net);
            }
            WiFi.scanDelete();
            _isScanning = false;
        } else if (n == WIFI_SCAN_FAILED) {
            _isScanning = false;
            Serial.println("WiFi: Scan failed.");
        }
    }

    // Check connection status
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
