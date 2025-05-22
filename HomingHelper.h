// HomingHelper.h
#pragma once

#include <ClearCore.h>

/// Parameters to drive a homing cycle
struct HomingParams {
    MotorDriver* motor;        // which driver
    float        stepsPerUnit; // steps per inch (or mm)
    uint32_t     fastVel;      // fast approach speed (steps/s)
    uint32_t     slowVel;      // slow approach speed (steps/s)
    uint32_t     dwellMs;      // dwell time before slow phase (ms)
    float        backoffUnits; // how far to back off (inches)
    uint32_t     timeoutMs;    // overall timeout (ms)
};

/// Drives a simplified 6-step homing sequence:
/// 1) fast approach, 2) dwell, 3) slow approach, 4) wait for HLFB/stop,
/// 5) back-off, 6) finalize/zero, or if already homed
/// move from arbitrary position back to zero (soft limit) then finalize.
class HomingHelper {
public:
    explicit HomingHelper(const HomingParams& p);

    /// Kick off a new homing. Returns false if already busy.
    bool start();

    /// Call every loop(). Steps through the state machine.
    void process();

    /// True while any phase is active.
    bool isBusy() const;

    /// True if homing timed out or failed.
    bool hasFailed() const;

private:
    enum class State {
        Idle,
        FastApproach,
        Dwell,
        SlowApproach,
        WaitForStop,
        Backoff,
        Finalize,
        Failed
    };

    HomingParams _p;
    State        _state;
    uint32_t     _stamp;         // ms when current phase began
    uint32_t     _startTime;     // ms when homing started (for logging)
    State        _lastState;     // last state logged
    bool         _hasBeenHomed;  // tracks if homed once
};
