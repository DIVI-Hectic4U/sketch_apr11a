#ifndef APP_STATE_H
#define APP_STATE_H

#include <Arduino.h>

/**
 * AppState Singleton
 * Holds the global state of the application, including WiFi status,
 * user metrics (XP, Spoons, Streak), and current session info.
 */
class AppState {
public:
    static AppState& getInstance() {
        static AppState instance;
        return instance;
    }

    // WiFi & Connection
    bool isWifiConnected = false;
    String localIP = "0.0.0.0";
    
    // User Metrics
    int xp = 0;
    int spoonsUsed = 0;
    int spoonsTotal = 12;
    int streakDays = 0;
    int level = 1;

    // Session Info
    String currentTaskName = "No active task";
    String currentSubtaskName = "---";
    int sessionRemainingSeconds = 0;
    bool isSessionRunning = false;

    // Device Info
    String deviceId = "";

private:
    AppState() {
        // Generate a basic device ID if needed (Part 6 will handle this properly)
        deviceId = "ESP32S3_" + String((uint32_t)ESP.getEfuseMac(), HEX);
    }
    
    // Prevent copying
    AppState(const AppState&) = delete;
    AppState& operator=(const AppState&) = delete;
};

#endif // APP_STATE_H
