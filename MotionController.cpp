// Updated MotionController.cpp for true unipolar PWM (0=zero, 255=max)
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
    lastUpdateTime(0)
{
}

void MotionController::setup() {
    ClearCore::ConnectorUsb.SendLine("======== Initializing MotionController ========");

    // Configure motor for velocity mode with proper clocking
    MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_NORMAL);
    MotorMgr.MotorModeSet(MotorManager::MOTOR_M0M1, Connector::CPM_MODE_A_DIRECT_B_PWM);
    ClearCore::ConnectorUsb.SendLine("Motor mode set to CPM_MODE_A_DIRECT_B_PWM");

    // Set the motor to initially disabled
    MOTOR_SPINDLE.EnableRequest(false);
    ClearCore::ConnectorUsb.SendLine("Motor initially disabled");

    // Set Input A to HIGH for velocity mode (required for MCVC operation)
    MOTOR_SPINDLE.MotorInAState(true);
    ClearCore::ConnectorUsb.SendLine("Input A set HIGH for velocity mode");

    // Initialize with zero velocity
    MOTOR_SPINDLE.MotorInBDuty(0);
    ClearCore::ConnectorUsb.SendLine("Initial duty cycle set to zero (0)");

    // Log default RPM from settings
    auto& settings = SettingsManager::Instance().settings();
    ClearCore::ConnectorUsb.Send("Default RPM from settings: ");
    ClearCore::ConnectorUsb.SendLine(static_cast<int32_t>(settings.defaultRPM));

    // Initialize timing
    lastUpdateTime = ClearCore::TimingMgr.Milliseconds();

    ClearCore::ConnectorUsb.SendLine("MotionController setup complete");
}

void MotionController::update() {
    unsigned long currentTime = ClearCore::TimingMgr.Milliseconds();

    // Only check safety relay state when spindle is running
    if (spindleRunning) {
        // Ensure safety relay is enabled when spindle is running
        if (!SAFETY_RELAY_OUTPUT.State()) {
            ClearCore::ConnectorUsb.SendLine("ERROR: Safety relay not enabled while spindle running");
            SAFETY_RELAY_OUTPUT.State(true);
        }
    }

    lastUpdateTime = currentTime;
}

void MotionController::StartSpindle(float rpm) {
    ClearCore::ConnectorUsb.SendLine("========================================");
    ClearCore::ConnectorUsb.Send("StartSpindle called with RPM: ");
    ClearCore::ConnectorUsb.SendLine(static_cast<int32_t>(rpm));

    // Check if safety relay is enabled
    if (!SAFETY_RELAY_OUTPUT.State()) {
        ClearCore::ConnectorUsb.SendLine("Cannot start spindle - safety relay disabled");
        return;
    }

    // Validate RPM range with minimum value to ensure movement
    if (rpm < 750) {
        ClearCore::ConnectorUsb.SendLine("Setting minimum RPM of 750");
        rpm = 750;
    }
    if (rpm > SPINDLE_MAX_RPM) {
        ClearCore::ConnectorUsb.SendLine("Capping RPM to maximum");
        rpm = SPINDLE_MAX_RPM;
    }

    // Set command value
    commandedRPM = rpm;

    // Debug info
    ClearCore::ConnectorUsb.Send("Final commanded RPM: ");
    ClearCore::ConnectorUsb.SendLine(static_cast<int32_t>(commandedRPM));

    // Ensure velocity mode is properly configured
    MOTOR_SPINDLE.MotorInAState(true);

    // Calculate motor duty cycle directly from commanded RPM
    // For true unipolar PWM: 0 = zero speed, 255 = max speed
    uint8_t dutyValue;
    if (commandedRPM > 0) {
        // Map RPM from [0, SPINDLE_MAX_RPM] to PWM duty [0, 255]
        float normalizedSpeed = commandedRPM / SPINDLE_MAX_RPM;  // 0.0 to 1.0
        dutyValue = static_cast<uint8_t>(normalizedSpeed * 255.0f);

        // Ensure valid range - minimum value must be above zero
        if (dutyValue < 20) dutyValue = 20;  // Minimum threshold to ensure movement
        if (dutyValue > 255) dutyValue = 255;

        ClearCore::ConnectorUsb.Send("Setting unipolar PWM duty to: ");
        ClearCore::ConnectorUsb.SendLine(static_cast<int32_t>(dutyValue));
    }
    else {
        dutyValue = 0;  // Zero velocity
        ClearCore::ConnectorUsb.SendLine("Setting zero velocity (duty=0)");
    }

    // Enable the motor and set velocity directly
    MOTOR_SPINDLE.EnableRequest(true);
    MOTOR_SPINDLE.MotorInBDuty(dutyValue);

    // Set running flag
    spindleRunning = true;

    ClearCore::ConnectorUsb.SendLine("Motor enabled - MSP handling acceleration");
    ClearCore::ConnectorUsb.SendLine("========================================");
}

void MotionController::StopSpindle() {
    ClearCore::ConnectorUsb.SendLine("========================================");
    ClearCore::ConnectorUsb.SendLine("StopSpindle called - letting MSP handle deceleration");

    // Set command values
    spindleRunning = false;
    commandedRPM = 0.0f;

    // Set zero velocity - send 0 for zero speed in this configuration
    MOTOR_SPINDLE.MotorInBDuty(0);
    ClearCore::ConnectorUsb.SendLine("Set duty cycle to 0 (zero velocity)");

    // Keep the motor enabled for a short time to ensure proper deceleration
    unsigned long startTime = ClearCore::TimingMgr.Milliseconds();
    while (ClearCore::TimingMgr.Milliseconds() - startTime < 1500) {
        // Allow time for deceleration
    }

    // Now disable the motor
    MOTOR_SPINDLE.EnableRequest(false);

    ClearCore::ConnectorUsb.SendLine("Spindle stopped and motor disabled");
    ClearCore::ConnectorUsb.SendLine("========================================");
}

bool MotionController::IsSpindleRunning() const {
    return spindleRunning;
}

float MotionController::CommandedRPM() const {
    return commandedRPM;
}

void MotionController::SetManualOverrideRPM(float rpm) {
    ClearCore::ConnectorUsb.SendLine("========================================");
    ClearCore::ConnectorUsb.Send("SetManualOverrideRPM: ");
    ClearCore::ConnectorUsb.SendLine(static_cast<int32_t>(rpm));

    // Validate RPM range
    if (rpm < 750) rpm = 750;  // Minimum RPM for reliable operation
    if (rpm > SPINDLE_MAX_RPM) rpm = SPINDLE_MAX_RPM;

    // Only update if the spindle is already running
    if (spindleRunning) {
        commandedRPM = rpm;

        // Calculate duty value for true unipolar PWM
        float normalizedSpeed = commandedRPM / SPINDLE_MAX_RPM;  // 0.0 to 1.0
        uint8_t dutyValue = static_cast<uint8_t>(normalizedSpeed * 255.0f);

        // Ensure valid range
        if (dutyValue < 20) dutyValue = 20;  // Minimum threshold
        if (dutyValue > 255) dutyValue = 255;  // Maximum value

        // Apply new velocity directly - MSP will handle ramping
        MOTOR_SPINDLE.MotorInBDuty(dutyValue);

        ClearCore::ConnectorUsb.Send("New PWM duty set: ");
        ClearCore::ConnectorUsb.SendLine(static_cast<int32_t>(dutyValue));
    }
    else {
        ClearCore::ConnectorUsb.SendLine("RPM stored but spindle not running");
        commandedRPM = rpm;  // Store for future use
    }
    ClearCore::ConnectorUsb.SendLine("========================================");
}

void MotionController::EmergencyStop() {
    // Immediately stop the spindle
    spindleRunning = false;
    commandedRPM = 0.0f;

    // Force the motor to zero velocity immediately
    MOTOR_SPINDLE.MotorInBDuty(0);  // Zero velocity (0% duty cycle)
    MOTOR_SPINDLE.EnableRequest(false);  // Disable motor

    // Inform user of emergency stop
    ClearCore::ConnectorUsb.SendLine("EMERGENCY STOP: Motor controller halted");
}
