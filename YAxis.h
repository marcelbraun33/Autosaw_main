#pragma once

#include <ClearCore.h>
#include "HomingHelper.h"
#include "Config.h"

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

    // Legacy compatibility
    void Home() { StartHoming(); }
    void ClearAlerts();
    void Jog(float deltaInches, float velocityScale);

    // Status queries
    float GetPosition() const;       // Commanded position [in]
    float GetActualPosition() const; // Feedback position [in]
    float GetTorquePercent() const;  // % estimated torque
    bool  IsMoving() const;
    bool  IsHomed() const;
    bool  IsHoming() const;

    // Motion tuning
    float GetStepsPerInch() const { return _stepsPerInch; }
    void  UpdateVelocity(float velocityScale);

    // Fault-recovery helpers
    bool ResetAndPrepare() {
        // Clear any alerts and enable motor
        ClearAlerts();
        _motor->EnableRequest(true);
        Delay_ms(100);
        // If not ready, try a disable/enable cycle
        if (_motor->StatusReg().bit.ReadyState != MotorDriver::MOTOR_READY) {
            _motor->EnableRequest(false);
            Delay_ms(100);
            _motor->EnableRequest(true);
            Delay_ms(100);
        }
        // Return true if motor is ready and no alerts remain
        return _motor->StatusReg().bit.ReadyState == MotorDriver::MOTOR_READY
            && !_motor->StatusReg().bit.AlertsPresent;
    }

    bool HasAlerts() const {
        return _motor && _motor->StatusReg().bit.AlertsPresent;
    }

private:
    // Internal state
    bool    _isSetup = false;
    bool    _isMoving = false;
    bool    _isHomed = false;
    bool    _hasBeenHomed = false;
    float   _currentPos = 0.0f;   // Commanded position [inches]
    float   _actualPos = 0.0f;   // Feedback position [inches]
    float   _targetPos = 0.0f;   // Target position [inches]
    float   _torquePct = 0.0f;   // 0–100%
    const float _stepsPerInch;
    MotorDriver* const _motor;      // Pointer to ClearCore motor driver

    // Homing parameters
    static constexpr float HOMING_BACKOFF_INCH = 0.125f;
    static constexpr uint32_t HOMING_TIMEOUT_MS = 30000;
    static constexpr float HOME_VEL_FAST = 5000.0f;
    static constexpr float HOME_VEL_SLOW = 1000.0f;

    HomingHelper* _homingHelper;
};
