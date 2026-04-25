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
    void startSession(String taskId, int durationMinutes);
    void stopSession();

    bool isConnected = false;
    void onEvent(WStype_t type, uint8_t * payload, size_t length);

private:
    APIClient() {}
    
    WebSocketsClient ws;
    void connect();
};

#endif // API_CLIENT_H
