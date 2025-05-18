// === RetractCycle.h ===
#pragma once

#include "MotionController.h"

/// Handles rapid retract motion on a specific axis
class RetractCycle {
public:
    /// Initialize with controller, axis (enum), and retract distance
    void begin(MotionController* controller, AxisId axisId, float retractDistance);

    /// Perform the retract in update loop
    void update();

    bool isFinished() const;

private:
    MotionController* _controller = nullptr;
    AxisId            _axisId = AXIS_X;
    float             _retractDistance = 0.0f;
    bool              _started = false;
};
