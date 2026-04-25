#ifndef WIFI_MANAGER_CUSTOM_H
#define WIFI_MANAGER_CUSTOM_H

#include <WiFi.h>

/**
 * WiFiManagerWrapper
 * Handles in-app scanning and connection management.
 */
class WiFiManagerWrapper {
public:
    static WiFiManagerWrapper& getInstance() {
        static WiFiManagerWrapper instance;
        return instance;
    }

    void init();
    void update();
    
    /**
     * Scans for nearby WiFi networks and populates AppState.
     */
    void scanNetworks();

    /**
     * Connects to a specific network.
     * @param ssid The SSID to connect to.
     * @param password The password for the network.
     */
    void connectTo(String ssid, String password);

    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }

private:
    WiFiManagerWrapper() {}
};

#endif // WIFI_MANAGER_CUSTOM_H
