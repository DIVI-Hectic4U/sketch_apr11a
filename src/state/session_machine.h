#ifndef SESSION_MACHINE_H
#define SESSION_MACHINE_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Session states per spec §5.3
// ---------------------------------------------------------------------------
enum class SessionState {
    IDLE,
    FOCUS,        // Countdown within planned duration
    HYPERFOCUS,   // Elapsed past planned; overflow arc grows
    BREAK,        // User took a break; base arc frozen
    DISENGAGED,   // BREAK timed out (30s); arc drains backward
    COMPLETED     // Session ended cleanly (stop button)
};

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static constexpr int HYPERFOCUS_CAP_MINUTES = 30; // §5.3: max overflow before auto-BREAK

// ---------------------------------------------------------------------------
// SessionMachine — Singleton local timer authority
// §5.3: device runs its own state machine; does not poll backend every second
// ---------------------------------------------------------------------------
class SessionMachine {
public:
    static SessionMachine& getInstance() {
        static SessionMachine instance;
        return instance;
    }

    // Lifecycle
    void start(int plannedMinutes);
    void pause();       // → DISENGAGED (user manually disengaged)
    void resume();      // DISENGAGED/BREAK → FOCUS (if within plan) or HYPERFOCUS
    void takeBreak();   // FOCUS/HYPERFOCUS → BREAK
    void stop();        // Any → IDLE (complete session)

    // Called from session_machine_task every 100ms (Core 0)
    void tick();

    // Getters
    SessionState getState()              const { return _state; }
    int          getPlannedSeconds()     const { return _plannedSec; }
    int          getElapsedSeconds()     const { return _elapsedSec; }
    int          getOverflowSeconds()    const;   // elapsed - planned (clamped >=0)
    int          getBreakIdleSeconds()   const { return _breakIdleSec; }
    int          getRemainingSeconds()   const;   // planned - elapsed (clamped >=0)

    // Formatted strings for the ring label
    String getTimeLabelString()          const;

    // Arc angles 0–360 for ring widget
    int    getBaseArcAngle()             const;     // base arc (FOCUS progress)
    int    getOverflowArcAngle()         const;     // overflow arc (HYPERFOCUS)

    // Called when toast "Continue" is tapped — stay in HYPERFOCUS
    void   dismissToast()   { _toastActive = false; }
    bool   isToastActive()  const { return _toastActive; }

    // Time the HyperFocus toast appeared (millis)
    unsigned long toastShownAt() const { return _toastShownAt; }

private:
    SessionMachine() { _reset(); }
    SessionMachine(const SessionMachine&) = delete;
    SessionMachine& operator=(const SessionMachine&) = delete;

    void _reset();
    void _transitionTo(SessionState next);
    void _postStateToBackend(SessionState s);

    SessionState  _state         = SessionState::IDLE;
    int           _plannedSec    = 0;
    int           _elapsedSec    = 0;
    int           _breakIdleSec  = 0;   // counts up while in BREAK (→ DISENGAGED at 30s)
    int           _disengagedSec = 0;   // counts up in DISENGAGED for decay animation

    unsigned long _lastTickMs    = 0;
    unsigned long _toastShownAt  = 0;
    bool          _toastActive   = false;
};

#endif // SESSION_MACHINE_H
