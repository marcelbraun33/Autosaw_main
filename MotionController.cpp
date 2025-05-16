#include "MotionController.h"
#include "SettingsManager.h"
#include "Config.h"
#include <ClearCore.h>

MotionController& MotionController::Instance() {
    static MotionController inst;
    return inst;
}

MotionController::MotionController() :
    spindleRunning(false),
    commandedRPM(0.0f),
    currentRPM(0.0f),
    lastUpdateTime(0)
{
}

void MotionController::setup() {
    // Configure the spindle motor (M0) for PWM control

    // Set the motor to initially disabled
    MOTOR_SPINDLE.EnableRequest(false);

    // Set Input A to HIGH for velocity mode
    MOTOR_SPINDLE.MotorInAState(true);

    // Initialize with zero velocity (mid-point of PWM range)
    MOTOR_SPINDLE.MotorInBDuty(127);

    // Initialize timing
    lastUpdateTime = ClearCore::TimingMgr.Milliseconds();
}

void MotionController::update() {
    // Get current time for ramping calculations
    unsigned long currentTime = ClearCore::TimingMgr.Milliseconds();
    float elapsedSec = (currentTime - lastUpdateTime) / 1000.0f;

    // Skip if elapsed time is too small or invalid
    if (elapsedSec <= 0 || elapsedSec > 1.0f) {
        lastUpdateTime = currentTime;
        return;
    }

    // Handle spindle ramping - using 1000 RPM/sec for acceleration/deceleration
    const float RAMP_RATE = 1000.0f;
    float maxRpmChange = RAMP_RATE * elapsedSec;

    if (spindleRunning) {
        // If we need to accelerate or decelerate to reach commanded RPM
        if (currentRPM < commandedRPM) {
            currentRPM += maxRpmChange;
            if (currentRPM > commandedRPM) {
                currentRPM = commandedRPM;
            }
        }
        else if (currentRPM > commandedRPM) {
            currentRPM -= maxRpmChange;
            if (currentRPM < commandedRPM) {
                currentRPM = commandedRPM;
            }
        }

        // Apply current RPM to motor by scaling to PWM duty cycle
        // Map RPM range (0 to SPINDLE_MAX_RPM) to PWM range (127±127)
        uint8_t dutyValue;
        if (currentRPM > 0) {
            // Scale positive RPM to 128-255 range
            dutyValue = 127 + (uint8_t)(currentRPM * 127.0f / SPINDLE_MAX_RPM);
            if (dutyValue > 255) dutyValue = 255;
        }
        else {
            // No negative RPM for now
            dutyValue = 127;
        }

        MOTOR_SPINDLE.MotorInBDuty(dutyValue);
    }
    else if (currentRPM > 0) {
        // Ramp down to stop
        currentRPM -= maxRpmChange;
        if (currentRPM <= 0) {
            currentRPM = 0;
            MOTOR_SPINDLE.MotorInBDuty(127);  // Set to zero speed
            MOTOR_SPINDLE.EnableRequest(false);  // Disable motor when fully stopped
        }
        else {
            // Still decelerating
            uint8_t dutyValue = 127 + (uint8_t)(currentRPM * 127.0f / SPINDLE_MAX_RPM);
            if (dutyValue > 255) dutyValue = 255;
            MOTOR_SPINDLE.MotorInBDuty(dutyValue);
        }
    }

    lastUpdateTime = currentTime;
}

void MotionController::StartSpindle(float rpm) {
    // Validate RPM range
    if (rpm < 0) rpm = 0;
    if (rpm > SPINDLE_MAX_RPM) rpm = SPINDLE_MAX_RPM;

    commandedRPM = rpm;
    spindleRunning = true;

    // Enable the motor hardware if not currently running
    if (currentRPM == 0) {
        MOTOR_SPINDLE.EnableRequest(true);
    }

    // Actual speed change will be handled in update() for smooth ramping
}

void MotionController::StopSpindle() {
    spindleRunning = false;
    commandedRPM = 0.0f;

    // Actual stopping will be handled in update() with ramping
}

bool MotionController::IsSpindleRunning() const {
    return spindleRunning;
}

float MotionController::CommandedRPM() const {
    if (spindleRunning) {
        return commandedRPM;
    }
    return 0.0f;
}

void MotionController::SetManualOverrideRPM(float rpm) {
    if (rpm < 0) rpm = 0;
    if (rpm > SPINDLE_MAX_RPM) rpm = SPINDLE_MAX_RPM;

    commandedRPM = rpm;

    // If spindle is running, update will handle the change
    // with proper acceleration/deceleration
}
void MotionController::EmergencyStop() {
    // Immediately stop the spindle without ramping
    spindleRunning = false;
    commandedRPM = 0.0f;
    currentRPM = 0.0f;
    
    // Force the motor to zero velocity immediately
    MOTOR_SPINDLE.MotorInBDuty(127);  // Center value = zero velocity
    MOTOR_SPINDLE.EnableRequest(false);
    
    // Inform user of emergency stop
    ClearCore::ConnectorUsb.SendLine("EMERGENCY STOP: Motor controller halted");
}
