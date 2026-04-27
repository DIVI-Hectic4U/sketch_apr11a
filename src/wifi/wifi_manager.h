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
     * Starts an asynchronous scan for nearby WiFi networks.
     */
    void scanNetworksAsync();

    /**
     * Connects to a specific network and saves credentials to NVS.
     * @param ssid The SSID to connect to.
     * @param password The password for the network.
     */
    void connectTo(String ssid, String password);

    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
    bool isScanning() const { return _isScanning; }

private:
    WiFiManagerWrapper() {}
    bool _isScanning = false;
};

#endif // WIFI_MANAGER_CUSTOM_H
