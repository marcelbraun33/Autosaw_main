#pragma once
#include <ClearCore.h>

/// Parameters to drive a homing cycle
struct HomingParams {
    MotorDriver* motor;        // which driver
    float        stepsPerUnit; // steps per inch (or mm)
    uint32_t     fastVel;      // fast approach speed (steps/s)
    uint32_t     slowVel;      // slow approach speed (steps/s)
    uint32_t     dwellMs;      // how long to wait before slow phase (ms)
    float        backoffUnits; // how far to back off (inches)
    uint32_t     timeoutMs;    // overall timeout (ms)
};

/// Drives a 6-step homing sequence:
/// 1) fast, 2) dwell, 3) slow, 4) wait for HLFB, 5) abrupt stop, 6) back-off, 7) zero.
/// On timeout or alert it aborts and returns to Idle.
class HomingHelper {
public:
    explicit HomingHelper(const HomingParams& p);

    /// Kick off a new homing.  Returns false if already busy.
    bool start();

    /// Call every loop().  Will step through the state machine.
    void process();

    /// True while any phase is active.
    bool isBusy() const;

    /// True if we timed out or were cancelled.
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
    uint32_t     _stamp;  // ms at state entry
};
#pragma once
