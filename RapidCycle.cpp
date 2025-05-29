// controllers/RapidCycle.cpp
#include "RapidCycle.h"

RapidCycle::RapidCycle(MotionController& mc_, float nextPos, float rapidSpeed)
    : mc(mc_), targetPos(nextPos), speed(rapidSpeed) {
}

void RapidCycle::start() {
    mc.getXAxis().moveTo(targetPos, speed);
}

void RapidCycle::stop() {
    mc.getXAxis().stop();
}

bool RapidCycle::isDone() const {
    return mc.isXAxisMotionComplete();
}

bool RapidCycle::hasError() const {
    return false;
}

CycleError RapidCycle::error() const {
    return CycleError::None;
}
