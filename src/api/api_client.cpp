#include "api_client.h"
#include <ArduinoJson.h>
#include "../state/app_state.h"
#include "../../config.h"

// Static trampoline
static void wsEventTrampoline(WStype_t type, uint8_t * payload, size_t length) {
    APIClient::getInstance().onEvent(type, payload, length);
}

void APIClient::init() {
    randomSeed(esp_random()); // Seed with hardware entropy
    Serial.println("WS: API Client initialized.");
}

void APIClient::connect() {
    AppState& state = AppState::getInstance();
    if (state.deviceToken.length() == 0 && state.userId.length() == 0) return;

    String path = (state.deviceToken.length() > 0) 
                  ? "/?deviceToken=" + state.deviceToken 
                  : "/?token=" + state.userId;

    ws.beginSSL("backend-node-ascent.onrender.com", 443, path.c_str());
    ws.onEvent(wsEventTrampoline);
    ws.setReconnectInterval(5000);
}

static bool wsInitialized = false;

void APIClient::update() {
    AppState& state = AppState::getInstance();
    if (!wsInitialized && state.isWifiConnected && state.userId.length() > 0) {
        connect();
        wsInitialized = true;
    }
    if (wsInitialized) ws.loop();
}

void APIClient::fetchDashboard() {
    if (isConnected) ws.sendTXT("{\"action\":\"fetch_dashboard\"}");
}

void APIClient::startSession(String taskId, int durationMinutes) {
    if (isConnected) {
        String msg = "{\"action\":\"start\",\"duration\":" + String(durationMinutes) + ",\"taskId\":\"" + taskId + "\"}";
        ws.sendTXT(msg);
    }
}

void APIClient::stopSession() {
    if (isConnected) ws.sendTXT("{\"action\":\"stop\"}");
}

void APIClient::completeSubtask(String subtaskId) {
    if (isConnected) {
        String msg = "{\"action\":\"complete_subtask\",\"subtaskId\":\"" + subtaskId + "\"}";
        ws.sendTXT(msg);
    }
}

void APIClient::pairDevice(String code) {
    if (isConnected) {
        String msg = "{\"action\":\"pair_init\",\"code\":\"" + code + "\"}";
        ws.sendTXT(msg);
    }
}

void APIClient::onEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            isConnected = false;
            break;
        case WStype_CONNECTED:
            isConnected = true;
            fetchDashboard();
            break;
        case WStype_TEXT: {
            // Allocate JSON document on the HEAP to save Stack space
            // For 1.8KB, we use a 4KB pool
            DynamicJsonDocument doc(4096); 
            DeserializationError error = deserializeJson(doc, payload, length);
            if (error) return;

            String msgType = doc["type"] | "";
            AppState& state = AppState::getInstance();

            if (msgType == "dashboard_update") {
                JsonObject pl = doc["payload"];
                JsonObject stats = pl["stats"];
                state.xp = stats["totalXP"] | 0;
                state.points = stats["pointsEarned"] | 0;
                state.level = stats["level"] | 1;
                state.streakDays = stats["currentStreak"] | 0;
                state.preferredBreakDuration = stats["preferredBreakDuration"] | 5;
                state.hyperFocusDuration = stats["hyperFocusDuration"] | 45;
                state.cycleCount = stats["cycleCount"] | 0;
                state.pomodoroMode = stats["pomodoroMode"] | "flexible";

                JsonObject spoons = stats["spoonState"];
                state.spoonsUsed = spoons["remaining"] | 12;
                state.spoonsTotal = spoons["total"] | 12;

                // CRITICAL: Clear existing tasks before sync to prevent duplication/leaks
                state.tasks.clear(); 
                
                JsonArray tasksArr = pl["tasks"];
                state.tasks.reserve(tasksArr.size());

                for (JsonObject t : tasksArr) {
                    // Skip tasks already marked completed
                    String status = t["status"] | "";
                    bool isCompleted = t["completed"] | false;
                    if (status == "completed" || isCompleted) continue;

                    TaskInfo task;
                    task.id = t["_id"] | "0";
                    task.name = t["title"] | "Untitled";
                    task.suggestedDuration = t["suggestedDuration"] | 25;
                    
                    // Parse subtasks
                    JsonArray subtasks = t["subtasks"];
                    String firstPending = "---";
                    bool firstFound = false;
                    
                    for (JsonObject st : subtasks) {
                        SubtaskInfo sub;
                        sub.id = st["_id"] | "";
                        sub.title = st["title"] | "Untitled";
                        sub.completed = st["completed"] | false;
                        
                        task.subtasks.push_back(sub);
                        
                        if (!sub.completed && !firstFound) {
                            firstPending = sub.title;
                            firstFound = true;
                        }
                    }
                    
                    task.subtask = firstPending;
                    task.progress = t["progress"] | 0;
                    
                    // Map string priority to int
                    String pStr = t["priority"] | "medium";
                    if (pStr == "high") task.priority = 2;
                    else if (pStr == "low") task.priority = 0;
                    else task.priority = 1;

                    state.tasks.push_back(task);
                }
                Serial.printf("WS: Dashboard synced (%d tasks)\n", state.tasks.size());
                state.dashboardDirty = true;
            }
            else if (msgType == "timer_update") {
                // Future use
            }
            else if (msgType == "pair_success") {
                String token = doc["payload"]["deviceToken"] | "";
                if (token.length() > 0) {
                    AppState& state = AppState::getInstance();
                    state.deviceToken = token;
                    state.save();
                    state.dashboardDirty = true;
                    Serial.println("WS: Device paired successfully! Saving token...");
                    // Force reconnection with new token on next update
                    wsInitialized = false;
                    ws.disconnect();
                }
            }
            break;
        }
        default: break;
    }
}
