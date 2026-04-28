#ifndef APP_STATE_H
#define APP_STATE_H

#include <Arduino.h>
#include <vector>
#include <Preferences.h>

struct ScannedNetwork {
    String ssid;
    int rssi;
    bool isOpen;
};

struct SubtaskInfo {
    String id;
    String title;
    bool completed;
};

/**
 * TaskInfo
 * Data structure representing a task in the dashboard.
 */
struct TaskInfo {
    String id;
    String name;
    String subtask; // title of first incomplete subtask for legacy compat
    std::vector<SubtaskInfo> subtasks;
    int progress; // 0-100
    int priority; // 0: low (blue), 1: med (yellow), 2: high (red)
    int suggestedDuration; // in minutes (calibrated from backend)
};

/**
 * AppState Singleton
 * Holds the global state of the application.
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
    String currentSSID = "Not Connected";
    std::vector<ScannedNetwork> scannedNetworks;
    
    // User Metrics
    int xp = 0;
    int spoonsUsed = 12;
    int spoonsTotal = 12;
    int streakDays = 0;
    int level = 1;
    int points = 0;
    int preferredBreakDuration = 5; // minutes
    int hyperFocusDuration = 45; // minutes
    int cycleCount = 0;
    String pomodoroMode = "flexible";

    // Session Info
    String currentTaskName = "No active task";
    String currentSubtaskName = "---";
    int sessionRemainingSeconds = 0;
    bool isSessionRunning = false;

    // Selection State
    String selectedTaskId = ""; // Now uses String ID
    String selectedSubtaskId = "";
    String selectedSubtaskTitle = "";

    // User Settings (Persistent)
    String userId = "default_user";
    String deviceId = "";
    String deviceToken = "";

    // Dashboard Data
    std::vector<TaskInfo> tasks;

    // Cross-core signaling flags (set on Core 1, consumed on Core 0)
    volatile bool dashboardDirty = false;

    void load() {
        Preferences prefs;
        prefs.begin("focusos", true);
        userId = prefs.getString("user_id", "default_user");
        deviceToken = prefs.getString("device_token", "");
        xp = prefs.getInt("xp", 0);
        level = prefs.getInt("level", 1);
        prefs.end();
        Serial.println("AppState: Data loaded from NVS.");
    }

    void save() {
        Preferences prefs;
        prefs.begin("focusos", false);
        prefs.putString("user_id", userId);
        prefs.putString("device_token", deviceToken);
        prefs.putInt("xp", xp);
        prefs.putInt("level", level);
        prefs.end();
        Serial.println("AppState: Data saved to NVS.");
    }

private:
    AppState() {
        deviceId = "ESP32S3_" + String((uint32_t)ESP.getEfuseMac(), HEX);
        load();
        
        // Mock data
        if (tasks.empty()) {
            tasks.push_back({"1", "Design system audit", "Define color tokens", 
                {{"s1", "Define color tokens", false}, {"s2", "Check contrast", false}}, 
                60, 2, 25});
            tasks.push_back({"2", "Write API docs", "Authentication flow", 
                {{"s3", "Authentication flow", false}, {"s4", "Error codes", false}}, 
                40, 1, 25});
            tasks.push_back({"3", "Backend cleanup", "Optimize queries", 
                {{"s5", "Optimize queries", false}}, 
                10, 0, 25});
        }
    }
    
    AppState(const AppState&) = delete;
    AppState& operator=(const AppState&) = delete;
};

#endif // APP_STATE_H
