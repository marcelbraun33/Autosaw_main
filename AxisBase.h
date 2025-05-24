#pragma once

#include <ClearCore.h>
#include "HomingHelper.h"

/// Configuration for a single linear axis
struct AxisConfig {
    float        stepsPerUnit;   // steps per inch (or unit)
    float        maxTravel;      // travel limit in inches
    HomingParams homeParams;     // homing settings
};

/// Shared base for X/Y/Z axes
class AxisBase {
public:
    AxisBase(MotorDriver* m, const AxisConfig& cfg)
        : _motor(m), _cfg(cfg), _homingHelper(cfg.homeParams) {
    }

    void Setup();
    void Update();
    bool StartHoming();
    bool MoveTo(float inches, float rate);

    /// Discrete step jog: one click = one step
    void JogStep(int32_t deltaClicks) {
        float stepSize = 1.0f / _cfg.stepsPerUnit;
        float newPos = _currentPos + deltaClicks * stepSize;
        if (newPos < 0.0f) newPos = 0.0f;
        else if (newPos > _cfg.maxTravel) newPos = _cfg.maxTravel;
        MoveTo(newPos, 1.0f);
    }
    // Add to AxisBase.h in the public section:

/// Continuously drive at velSteps (steps/sec), but respect limits
    void JogVelocity(int32_t velSteps) {
        // Make sure motor is enabled
        _motor->EnableRequest(true);

        // 1. If we're already at or beyond a limit, refuse to move further:
        if (velSteps < 0 && _currentPos <= 0.0f) {
            _motor->MoveStopAbrupt();
            _isMoving = false;
            return;
        }
        if (velSteps > 0 && _currentPos >= _cfg.maxTravel) {
            _motor->MoveStopAbrupt();
            _isMoving = false;
            return;
        }

        // 2. Send a continuous-velocity command to the motor:
        _motor->MoveVelocity(velSteps);
        _isMoving = (velSteps != 0);
    }

    /// Clear any motor alerts (shared)
    void ClearAlerts() {
        if (_motor->StatusReg().bit.AlertsPresent)
            _motor->ClearAlerts();
    }

    /// Emergency stop (shared)
    void EmergencyStop() {
        _motor->MoveStopAbrupt();
        _isMoving = false;
    }

    /// Smooth stop (shared)
    void Stop() {
        _motor->MoveStopDecel();
        _isMoving = false;
    }

    // status queries...
    float GetPosition() const { return _currentPos; }
    bool  IsMoving()    const { return _isMoving; }
    bool  IsHomed()     const { return _isHomed; }
    bool  IsHoming()    const { return _homingHelper.isBusy(); }
    
protected:
    MotorDriver* _motor;
    AxisConfig   _cfg;
    HomingHelper _homingHelper;
private:

    bool    _isSetup = false;
    bool    _isMoving = false;
    bool    _isHomed = false;
    int32_t _curSteps = 0;
    float   _currentPos = 0.0f;
};