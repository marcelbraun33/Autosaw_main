// TorqueSensor.h
#pragma once
#include <ClearCore.h>
// Don't include <algorithm> to avoid conflicts with min/max macros

class TorqueSensor {
public:
    static TorqueSensor& Instance() {
        static TorqueSensor instance;
        return instance;
    }

    void init(MotorDriver* motor) {
        _motor = motor;
        if (_motor) {
            // Configure HLFB for bipolar PWM (measured torque)
            _motor->HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
            _motor->HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
        }
    }

    void update() {
        if (!_motor) return;

        // Read torque measurement if available
        if (_motor->HlfbState() == MotorDriver::HLFB_HAS_MEASUREMENT) {
            // Get the latest measurement (-100% to +100%)
            float rawTorque = _motor->HlfbPercent();

            // Use magnitude - we don't care about direction
            float torqueMagnitude = rawTorque < 0 ? -rawTorque : rawTorque; // abs() equivalent

            // Apply filtering: newFiltered = α×raw + (1-α)×oldFiltered
            _filteredTorque = _alpha * torqueMagnitude + (1.0f - _alpha) * _filteredTorque;
        }
    }

    float getFilteredTorque() const {
        return _filteredTorque; // 0-100%
    }

    void setFilterCoefficient(float alpha) {
        // Clamp alpha to valid range [0.01, 1.0] without using std::min/max
        if (alpha < 0.01f) {
            _alpha = 0.01f;
        }
        else if (alpha > 1.0f) {
            _alpha = 1.0f;
        }
        else {
            _alpha = alpha;
        }
    }

private:
    TorqueSensor() : _motor(nullptr), _filteredTorque(0.0f), _alpha(0.1f) {}

    MotorDriver* _motor;    // Pointer to the motor driver
    float _filteredTorque;  // Filtered torque value (0-100%)
    float _alpha;           // Filter coefficient (0.1 = 10% new, 90% old)
};
