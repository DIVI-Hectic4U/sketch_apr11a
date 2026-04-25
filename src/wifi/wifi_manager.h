#ifndef WIFI_MANAGER_CUSTOM_H
#define WIFI_MANAGER_CUSTOM_H

#include <WiFi.h>

/**
 * WiFiManagerWrapper
 * Handles non-blocking WiFi connection and status reporting.
 */
class WiFiManagerWrapper {
public:
    static WiFiManagerWrapper& getInstance() {
        static WiFiManagerWrapper instance;
        return instance;
    }

    /**
     * Initializes WiFi and starts a non-blocking connection attempt.
     */
    void init();

    /**
     * Periodic update function to check status and handle reconnection.
     */
    void update();

    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }

private:
    WiFiManagerWrapper() {}
    unsigned long lastCheck = 0;
};

#endif // WIFI_MANAGER_CUSTOM_H
