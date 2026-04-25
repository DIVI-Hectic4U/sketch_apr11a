#include "session_machine.h"
#include "app_state.h"

void SessionMachine::start(int minutes) {
    totalSeconds = minutes * 60;
    remainingSeconds = totalSeconds;
    state = SessionState::RUNNING;
    lastTick = millis();
    
    AppState& app = AppState::getInstance();
    app.isSessionRunning = true;
    app.sessionRemainingSeconds = remainingSeconds;
}

void SessionMachine::pause() {
    if (state == SessionState::RUNNING) {
        state = SessionState::PAUSED;
    }
}

void SessionMachine::resume() {
    if (state == SessionState::PAUSED) {
        state = SessionState::RUNNING;
        lastTick = millis();
    }
}

void SessionMachine::stop() {
    state = SessionState::IDLE;
    remainingSeconds = 0;
    AppState::getInstance().isSessionRunning = false;
}

void SessionMachine::update() {
    if (state != SessionState::RUNNING) return;

    if (millis() - lastTick >= 1000) {
        lastTick += 1000;
        if (remainingSeconds > 0) {
            remainingSeconds--;
            AppState::getInstance().sessionRemainingSeconds = remainingSeconds;
        } else {
            onComplete();
        }
    }
}

void SessionMachine::onComplete() {
    state = SessionState::COMPLETED;
    AppState& app = AppState::getInstance();
    app.isSessionRunning = false;
    
    // Part 5.2 requirement: consume 1 spoon
    if (app.spoonsUsed > 0) {
        app.spoonsUsed--;
    }
    app.xp += 50; // Completion bonus
}

String SessionMachine::getTimeString() const {
    int m = remainingSeconds / 60;
    int s = remainingSeconds % 60;
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
    return String(buf);
}
