﻿// Spindle.cpp
#include "Spindle.h"
#include "Config.h"
#include <ClearCore.h>

Spindle::Spindle() : running(false), commandedRPM(0.0f) {}

// motion/Spindle.cpp
#include <ClearCore.h>
#include "Spindle.h"
#include "Config.h"

// In Spindle.cpp, update the Setup() method:
void Spindle::Setup() {
    // Put both M0 & M1 into velocity mode globally
    MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_NORMAL);
    MotorMgr.MotorModeSet(
        MotorManager::MOTOR_M0M1,
        Connector::CPM_MODE_A_DIRECT_B_PWM
    );

    // Configure HLFB to read bipolar PWM signal for torque feedback
    MOTOR_SPINDLE.HlfbMode(ClearCore::MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);

    // Set appropriate HLFB carrier frequency (482 Hz is common for ClearPath motors)
    MOTOR_SPINDLE.HlfbCarrier(ClearCore::MotorDriver::HLFB_CARRIER_482_HZ);

    // Set active high HLFB level (ensure this matches your motor configuration)
    MOTOR_SPINDLE.HlfbActiveLevel(true);

    // Set a reasonable HLFB filter length (helps with noise)
    MOTOR_SPINDLE.HlfbFilterLength(3);

    // Now idle the outputs
    MOTOR_SPINDLE.EnableRequest(false);
    MOTOR_SPINDLE.MotorInAState(true);
    MOTOR_SPINDLE.MotorInBDuty(0);

    ClearCore::ConnectorUsb.SendLine("[Spindle] Setup complete");
}




void Spindle::Start(float rpm) {
    if (rpm < 750) rpm = 750;
    if (rpm > SPINDLE_MAX_RPM) rpm = SPINDLE_MAX_RPM;
    commandedRPM = rpm;

    float normalizedSpeed = commandedRPM / SPINDLE_MAX_RPM;
    uint8_t dutyValue = static_cast<uint8_t>(normalizedSpeed * 255.0f);
    if (dutyValue < 20) dutyValue = 20;
    if (dutyValue > 255) dutyValue = 255;

    MOTOR_SPINDLE.EnableRequest(true);
    MOTOR_SPINDLE.MotorInBDuty(dutyValue);
    running = true;
}

void Spindle::Stop() {
    running = false;
    commandedRPM = 0.0f;
    MOTOR_SPINDLE.MotorInBDuty(0);
    unsigned long startTime = ClearCore::TimingMgr.Milliseconds();
    while (ClearCore::TimingMgr.Milliseconds() - startTime < 1500) {}
    MOTOR_SPINDLE.EnableRequest(false);
}

void Spindle::SetRPM(float rpm) {
    if (rpm < 750) rpm = 750;
    if (rpm > SPINDLE_MAX_RPM) rpm = SPINDLE_MAX_RPM;
    commandedRPM = rpm;
    if (running) {
        float normalizedSpeed = commandedRPM / SPINDLE_MAX_RPM;
        uint8_t dutyValue = static_cast<uint8_t>(normalizedSpeed * 255.0f);
        if (dutyValue < 20) dutyValue = 20;
        if (dutyValue > 255) dutyValue = 255;
        MOTOR_SPINDLE.MotorInBDuty(dutyValue);
    }
}

bool Spindle::IsRunning() const {
    return running;
}

float Spindle::CommandedRPM() const {
    return commandedRPM;
}

void Spindle::EmergencyStop() {
    running = false;
    commandedRPM = 0.0f;
    MOTOR_SPINDLE.MotorInBDuty(0);
    MOTOR_SPINDLE.EnableRequest(false);
}
