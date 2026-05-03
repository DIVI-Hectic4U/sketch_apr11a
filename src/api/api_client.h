#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <Arduino.h>
#include <WebSocketsClient.h>

/**
 * APIClient
 * Handles all communication with the backend via WebSockets.
 * Uses the "WebSockets" library by Markus Sattler (Links2004).
 */
class APIClient {
public:
    static APIClient& getInstance() {
        static APIClient instance;
        return instance;
    }

    void init();
    void update(); // Call in loop()

    void fetchDashboard();
    void startSession(String taskId, String subtaskId, String subtaskTitle, int durationMinutes);
    void pauseSession();
    void resumeSession();
    void stopSession();
    void completeSubtask(String subtaskId);
    void pairDevice(String code);
    void forceReconnect();

    bool isConnected = false;
    void onEvent(WStype_t type, uint8_t * payload, size_t length);

private:
    APIClient() {}
    
    WebSocketsClient ws;
    void connect();

    // Stale token detection: track consecutive disconnects before first connect
    bool _everConnected = false;       // true once WStype_CONNECTED fires
    uint8_t _disconnectCount = 0;      // resets on successful connect
    unsigned long _lastDisconnectMs = 0;
};

#endif // API_CLIENT_H
