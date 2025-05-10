#include "MotionController.h"
#include "SettingsManager.h"

MotionController& MotionController::Instance() {
    static MotionController inst;
    return inst;
}

MotionController::MotionController() = default;

void MotionController::setup() {
    // Placeholder for motor initialization logic
}

void MotionController::update() {
    // Placeholder for motor runtime update logic
}

void MotionController::StartSpindle(float rpm) {
    commandedRPM = rpm;
    spindleRunning = true;

    // TODO: Replace with actual motor control command
}

void MotionController::StopSpindle() {
    spindleRunning = false;

    // TODO: Replace with actual motor stop command
}

bool MotionController::IsSpindleRunning() const {
    return spindleRunning;
}

float MotionController::CommandedRPM() const {
    if (spindleRunning) {
        return SettingsManager::Instance().settings().defaultRPM;
    }
    return 0.0f;
}

void MotionController::SetManualOverrideRPM(float rpm) {
    if (rpm < 0) rpm = 0;
    if (rpm > 4000) rpm = 4000;
    commandedRPM = rpm;
}

