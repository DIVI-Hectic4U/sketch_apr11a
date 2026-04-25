#ifndef SESSION_MACHINE_H
#define SESSION_MACHINE_H

#include <Arduino.h>

enum class SessionState {
    IDLE,
    RUNNING,
    PAUSED,
    COMPLETED
};

/**
 * SessionMachine
 * Handles the countdown timer logic and session state transitions.
 */
class SessionMachine {
public:
    static SessionMachine& getInstance() {
        static SessionMachine instance;
        return instance;
    }

    void start(int minutes);
    void pause();
    void resume();
    void stop();
    void update(); // Called every loop

    SessionState getState() const { return state; }
    int getRemainingSeconds() const { return remainingSeconds; }
    int getTotalSeconds() const { return totalSeconds; }
    String getTimeString() const;

private:
    SessionMachine() : state(SessionState::IDLE), remainingSeconds(0), totalSeconds(0), lastTick(0) {}
    
    SessionState state;
    int remainingSeconds;
    int totalSeconds;
    unsigned long lastTick;

    void onComplete();
};

#endif // SESSION_MACHINE_H
