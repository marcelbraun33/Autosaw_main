#pragma once

#include <ClearCore.h>
#include "HomingHelper.h"

// Forward declaration
class DynamicFeed;

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
    float GetTorquePercent() const;   // % estimated torque (filtered value)
    bool IsMoving() const;
    bool IsHomed() const;
    bool IsHoming() const;

    // Legacy compatibility
    void Home() { StartHoming(); }
    void ClearAlerts();

    // Torque control methods - now delegate to DynamicFeed
    void SetTorqueTarget(float targetPercent);
    float GetTorqueTarget() const;
    bool StartTorqueControlledFeed(float targetPosition, float initialVelocityScale);
    void UpdateFeedRate(float newVelocityScale);
    bool IsInTorqueControlledFeed() const;
    void AbortTorqueControlledFeed();

    // Add public method for debugging (optional, but useful for unit tests or external checks)
    float DebugGetCurrentFeedRate() const;
    float DebugGetTorquePercent() const;
    bool DebugIsInTorqueControlFeed() const;

    // Add these methods for feed hold integration
    void PauseTorqueControlledFeed();
    void ResumeTorqueControlledFeed();

private:
    // Internal state
    bool    _isSetup = false;
    bool    _isMoving = false;
    bool    _isHomed = false;
    float   _currentPos = 0.0f;   // Inches
    float   _targetPos = 0.0f;    // Inches
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

    // Dynamic feed controller (handles torque-controlled feed)
    DynamicFeed* _dynamicFeed;
    
    // Legacy variable (needed by Update() and motion control logic)
    float _torquePct = 0.0f;    // 0–100%
};
