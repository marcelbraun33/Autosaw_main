
// === MPGJogManager.h ===
#pragma once
#include "MotionController.h"

/// Simple MPG/QDC jog wheel support for an axis
class MPGJogManager {
public:
    /// controller and axis (enum)
    void begin(MotionController* controller, AxisId axisId);
    /// poll encoder and move if turned
    void update();

private:
    MotionController* _controller = nullptr;
    AxisId            _axisId = AXIS_X;
    int32_t           _lastDetent = 0;
    static constexpr float JOG_STEP = 0.01f; // inches or degrees per click
};
