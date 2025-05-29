// controllers/RapidCycle.h
#pragma once

#include "ArduinoCompat.h"  
#include "ICycle.h"
#include "MotionController.h"


/// Rapid‐index the X‐axis to the next slice start point
class RapidCycle : public ICycle {
public:
    RapidCycle(MotionController& mc, float nextPos, float rapidSpeed);

    void start() override;
    void stop() override;
    bool isDone() const override;
    bool hasError() const override;
    CycleError error() const override;

private:
    MotionController& mc;
    float targetPos;
    float speed;
};
