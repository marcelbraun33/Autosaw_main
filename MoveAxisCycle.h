#pragma once

#include "ICycle.h"
#include "MotionController.h"

/// Cycle to move an axis to a target position at a given speed
class MoveAxisCycle : public ICycle {
public:
    MoveAxisCycle(MotionController &mc, AxisId axis, float target, float speed);

    void start() override;
    void stop() override;
    bool isDone() const override;
    bool hasError() const override;
    CycleError error() const override;

private:
    MotionController &_mc;
    AxisId _axis;
    float _target;
    float _speed;
    bool _started = false;
    bool _stopped = false;
    bool _error = false;
    CycleError _errorCode = CycleError::None;
};
