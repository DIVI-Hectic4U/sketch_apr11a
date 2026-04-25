#ifndef APP_STATE_H
#define APP_STATE_H

#include <Arduino.h>
#include <vector>

/**
 * TaskInfo
 * Data structure representing a task in the dashboard.
 */
struct TaskInfo {
    int id;
    String name;
    String subtask;
    int progress; // 0-100
    int priority; // 0: low (blue), 1: med (yellow), 2: high (red)
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
    
    // User Metrics
    int xp = 2450;
    int spoonsUsed = 8;
    int spoonsTotal = 12;
    int streakDays = 14;
    int level = 4;

    // Session Info
    String currentTaskName = "No active task";
    String currentSubtaskName = "---";
    int sessionRemainingSeconds = 0;
    bool isSessionRunning = false;

    // Dashboard Data
    std::vector<TaskInfo> tasks;

    // Device Info
    String deviceId = "";

private:
    AppState() {
        deviceId = "ESP32S3_" + String((uint32_t)ESP.getEfuseMac(), HEX);
        
        // Mock data for initial UI testing
        tasks.push_back({1, "Design system audit", "Define color tokens", 60, 2});
        tasks.push_back({2, "Write API docs", "Authentication flow", 40, 1});
        tasks.push_back({3, "Backend cleanup", "Optimize queries", 10, 0});
    }
    
    AppState(const AppState&) = delete;
    AppState& operator=(const AppState&) = delete;
};

#endif // APP_STATE_H
