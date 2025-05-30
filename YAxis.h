// YAxis.h
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

    // Status queries
    float GetPosition() const;        // Inches
    float GetTorquePercent() const;   // % estimated torque
    bool IsMoving() const;
    bool IsHomed() const;
    bool IsHoming() const;

    // In YAxis.h
    float GetStepsPerInch() const { return _stepsPerInch; }

    // FIXED VERSION:
    void UpdateVelocity(float velocityScale) {
        if (_isMoving) {
            // Use a proper value for regular moves, not homing
            _motor->VelMax(static_cast<uint32_t>(10000.0f * velocityScale)); // Use MAX_VELOCITY_Y
        }
    }
    // Add this to your YAxis class:
    bool ResetAndPrepare() {
        // Clear any alerts
        ClearAlerts();

        // Ensure motor is enabled
        _motor->EnableRequest(true);

        // Wait a short time for enable to take effect
        Delay_ms(100);

        // Check if the motor is ready
        if (_motor->StatusReg().bit.ReadyState != MotorDriver::MOTOR_READY) {
            // Try a fault recovery procedure
            _motor->EnableRequest(false);
            Delay_ms(100);
            _motor->EnableRequest(true);
            Delay_ms(100);
        }

        // Return success if motor is ready for motion
        return _motor->StatusReg().bit.ReadyState == MotorDriver::MOTOR_READY
            && !_motor->StatusReg().bit.AlertsPresent;
    }

    // Add this method to YAxis.h
    bool HasAlerts() const {
        return _motor && _motor->StatusReg().bit.AlertsPresent;
    }



    // Legacy compatibility
    void Home() { StartHoming(); }
    void ClearAlerts();
    void Jog(float deltaInches, float velocityScale);


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
};
