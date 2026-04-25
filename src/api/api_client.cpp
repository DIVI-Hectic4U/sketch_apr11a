#include "api_client.h"
#include <ArduinoJson.h>
#include "../state/app_state.h"
#include "../../config.h"

// Static trampoline so the C-style callback can reach our instance method
static void wsEventTrampoline(WStype_t type, uint8_t * payload, size_t length) {
    APIClient::getInstance().onEvent(type, payload, length);
}

void APIClient::init() {
    // Don't connect here — WiFi isn't ready yet at boot.
    // Connection will happen in update() once WiFi is available.
    Serial.println("WS: API Client initialized. Waiting for WiFi...");
}

void APIClient::connect() {
    AppState& state = AppState::getInstance();
    if (state.userId.length() == 0) {
        Serial.println("WS: No userId set. Skipping connection.");
        return;
    }

    String path = "/?token=" + state.userId;

    Serial.printf("WS: beginSSL to backend-node-ascent.onrender.com:443 path=%s\n", path.c_str());

    // beginSSL handles wss:// connections with port 443
    ws.beginSSL("backend-node-ascent.onrender.com", 443, path.c_str());
    ws.onEvent(wsEventTrampoline);
    ws.setReconnectInterval(5000);
}

static bool wsInitialized = false;

void APIClient::update() {
    AppState& state = AppState::getInstance();
    
    // Deferred connection: wait until WiFi is ready, then connect once
    if (!wsInitialized && state.isWifiConnected && state.userId.length() > 0) {
        Serial.println("WS: WiFi is ready. Initiating connection...");
        connect();
        wsInitialized = true;
    }
    
    if (wsInitialized) {
        ws.loop();
    }
}

void APIClient::fetchDashboard() {
    if (!isConnected) return;
    ws.sendTXT("{\"action\":\"fetch_dashboard\"}");
}

void APIClient::startSession(String taskId, int durationMinutes) {
    if (!isConnected) return;
    String msg = "{\"action\":\"start\",\"duration\":" + String(durationMinutes) + ",\"taskId\":\"" + taskId + "\"}";
    ws.sendTXT(msg);
}

void APIClient::stopSession() {
    if (!isConnected) return;
    ws.sendTXT("{\"action\":\"stop\"}");
}

void APIClient::onEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("WS: Disconnected.");
            isConnected = false;
            break;

        case WStype_CONNECTED:
            Serial.printf("WS: Connected to %s\n", (char*)payload);
            isConnected = true;
            fetchDashboard(); // Get initial data on connect
            break;

        case WStype_TEXT: {
            Serial.printf("WS: Received %u bytes\n", length);

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload, length);
            if (error) {
                Serial.printf("WS: JSON parse error: %s\n", error.c_str());
                return;
            }

            String msgType = doc["type"] | "";
            AppState& state = AppState::getInstance();

            if (msgType == "dashboard_update") {
                JsonObject pl = doc["payload"];
                JsonObject stats = pl["stats"];
                state.xp = stats["pointsEarned"] | 0;
                state.level = stats["level"] | 1;
                state.streakDays = stats["currentStreak"] | 0;

                JsonObject spoons = stats["spoonState"];
                state.spoonsUsed = spoons["remaining"] | 12;
                state.spoonsTotal = spoons["budget"] | 12;

                state.tasks.clear();
                JsonArray tasksArr = pl["tasks"];
                for (JsonObject t : tasksArr) {
                    TaskInfo task;
                    task.id = 0;
                    task.name = t["title"] | "Untitled";
                    JsonArray subtasks = t["subtasks"];
                    if (subtasks.size() > 0) {
                        task.subtask = subtasks[0]["title"] | "---";
                    } else {
                        task.subtask = "---";
                    }
                    task.progress = t["progress"] | 0;
                    state.tasks.push_back(task);
                }
                Serial.printf("WS: Dashboard synced (%d tasks)\n", state.tasks.size());
            }
            else if (msgType == "timer_update") {
                Serial.println("WS: Timer update received.");
            }
            break;
        }

        case WStype_PING:
            Serial.println("WS: Ping");
            break;

        case WStype_PONG:
            break;

        default:
            break;
    }
}
