// YAxis.h updated with torque control support
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

    // Torque control methods
    void SetTorqueTarget(float targetPercent);
    float GetTorqueTarget() const;
    bool StartTorqueControlledFeed(float targetPosition, float initialVelocityScale);
    void UpdateFeedRate(float newVelocityScale);
    bool IsInTorqueControlledFeed() const;
    void AbortTorqueControlledFeed();

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
    bool _hasBeenHomed = false;    // gets flipped true when homing succeeds

    // Torque control parameters
    bool _inTorqueControlFeed = false;
    float _torqueTarget = 70.0f;    // Default target torque (%)
    float _currentFeedRate = 1.0f;  // Current feed rate (0.0-1.0)
    float _maxFeedRate = 1.0f;      // Maximum feed rate for torque control
    float _minFeedRate = 0.1f;      // Minimum feed rate (10%)

    // Torque reading and control methods
    void UpdateTorqueMeasurement();
    void AdjustFeedRateBasedOnTorque();

    // PID-like control variables for smoother torque control
    float _torqueErrorAccumulator = 0.0f;
    uint32_t _lastTorqueUpdateTime = 0;
    static constexpr float TORQUE_Kp = 0.01f;  // Proportional gain
    static constexpr float TORQUE_Ki = 0.002f; // Integral gain 
    static constexpr float TORQUE_Kd = 0.005f; // Derivative gain
};
