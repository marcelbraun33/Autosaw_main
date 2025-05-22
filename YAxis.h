// YAxis.h
#pragma once

#include <ClearCore.h>
#include "HomingHelper.h"

class YAxis {
public:
    YAxis();
    ~YAxis();

    // Initialize motor driver and limits
    void Setup();

    // Call every loop() to update internal state and homing state machine
    void Update();

    // Start non-blocking homing sequence
    bool StartHoming();

    // Move control
    bool MoveTo(float positionInches, float velocityScale);
    void Stop();
    void EmergencyStop();

    // Status queries
    float GetPosition() const;        // Inches
    float GetTorquePercent() const;   // % estimated torque
    bool IsMoving() const;
    bool IsHomed() const;
    bool IsHoming() const;

    // Legacy compatibility
    void Home() { StartHoming(); }
    void ClearAlerts();

private:
    // Internal state
    bool    _isSetup = false;
    bool    _isMoving = false;
    bool    _isHomed = false;
    float   _currentPos = 0.0f;   // Inches
    float   _targetPos = 0.0f;    // Inches
    float   _torquePct = 0.0f;    // 0–100%
    const float _stepsPerInch;
    MotorDriver* const _motor;    // Pointer to ClearCore motor driver

    // Homing parameters
    static constexpr float HOMING_BACKOFF_INCH = 0.125f;
    static constexpr uint32_t HOMING_TIMEOUT_MS = 30000;
    static constexpr float HOME_VEL_FAST = 5000.0f;
    static constexpr float HOME_VEL_SLOW = 1000.0f;

    // Homing helper
    HomingHelper* _homingHelper;
};
