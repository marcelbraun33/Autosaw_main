// === FeedCycle.h ===
#pragma once
#include "MotionController.h"

/// Controlled feed motion with pressure feedback
class FeedCycle {
public:
    /// Initialize with controller, axis (enum), and target position
    void begin(MotionController* controller, AxisId axisId, float targetPos);
    /// Called every loop to adjust and drive
    void update();
    void setManualOverride(float value);
    float getEffectiveOverride() const;
    bool isFinished() const;
    bool hasError() const;

private:
    MotionController* _controller = nullptr;
    AxisId            _axisId = AXIS_X;
    float             _target = 0.0f;
    float             _manualOverride = 1.0f;
    float             _autoAdjustment = 1.0f;
    float             _pressureThreshold = 80.0f;

    float readTorque();
    void  applyPressureCompensation(float torque);
};
