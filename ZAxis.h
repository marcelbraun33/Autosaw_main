// ZAxis.h
#pragma once

#include <ClearCore.h>

class ZAxis {
public:
    ZAxis();

    // Initialize motor driver and limits
    void Setup();

    // Call every loop() to update internal state and homing state machine
    void Update();

    // Start non-blocking homing sequence
    bool StartHoming();

    // Move control (position in degrees)
    bool MoveTo(float positionDeg, float velocityScale);
    void Stop();
    void EmergencyStop();

    // Status queries
    float GetPosition() const;        // Degrees
    float GetTorquePercent() const;   // % estimated torque
    bool IsMoving() const;
    bool IsHomed() const;
    bool IsHoming() const;

    // Legacy compatibility
    void Home() { StartHoming(); }

private:
    // Homing state machine
    enum class HomingState {
        Idle,
        ApproachFast,
        ApproachSlow,
        WaitingForHardStop,
        Backoff,
        Complete,
        Failed
    } _homingState = HomingState::Idle;

    // Homing parameters
    static constexpr float HOMING_BACKOFF_DEG = 1.0f;      // degrees to back off
    static constexpr uint32_t HOMING_TIMEOUT_MS = 30000;
    static constexpr float HOME_VEL_FAST = 1000.0f;   // steps/s
    static constexpr float HOME_VEL_SLOW = 200.0f;    // steps/s

    // Internal state
    bool    _isSetup = false;
    bool    _isMoving = false;
    bool    _isHomed = false;
    float   _currentPos = 0.0f;      // Degrees
    float   _targetPos = 0.0f;      // Degrees
    float   _torquePct = 0.0f;      // 0–100%
    const float _stepsPerDeg;
    MotorDriver* const _motor;

    // Homing helpers
    uint32_t _homingStartTime = 0;
    int32_t  _backoffSteps = 0;

    void processHoming();
};
