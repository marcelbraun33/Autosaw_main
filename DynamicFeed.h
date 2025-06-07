#pragma once

#include <ClearCore.h>

class YAxis; // Forward declaration

/**
 * DynamicFeed module - Handles torque-controlled feed operations for the Y axis
 *
 * This class encapsulates logic for torque feedback-based feed rate control,
 * primarily used during cutting operations where constant torque is desirable.
 * Now also supports automatic retract to the start position after feed-to-stop.
 */
class DynamicFeed {
public:
    DynamicFeed(YAxis* owner, float stepsPerInch, MotorDriver* motor);
    ~DynamicFeed();

    // Start a torque-controlled feed operation
    bool start(float targetPosition, float initialVelocityScale);

    // Abort an in-progress operation
    void abort();

    // Update the dynamic feed controller (call regularly)
    // Returns true if the entire feed+retract cycle is complete
    bool update(float currentPos);

    // Check if a torque-controlled feed operation is active
    bool isActive() const;

    // Set the target torque for the feed controller
    void setTorqueTarget(float targetPercent);

    // Get the current torque target value
    float getTorqueTarget() const;

    // Update the current feed rate directly
    void updateFeedRate(float newVelocityScale);

    // Update the torque measurement from the motor
    float updateTorqueMeasurement();

    // Get the current measured torque value
    float getTorquePercent() const;

    // Get the current feed rate
    float getCurrentFeedRate() const;

    // Set the rapid (retract) velocity scale (0.0-1.0)
    void setRapidVelocityScale(float scale);

    // Get the rapid (retract) velocity scale
    float getRapidVelocityScale() const;
    
    // Pause the active feed operation
    void pause();
    
    // Resume a paused feed operation
    void resume();
    
    // Check if feed is currently paused
    bool isPaused() const;

private:
    enum class State {
        Idle,
        Feeding,
        Retracting,
        Paused    // Add this state for feed hold support
    };

    YAxis* _owner;
    MotorDriver* const _motor;
    const float _stepsPerInch;

    // Feed state
    State _state = State::Idle;
    float _targetPos = 0.0f;
    float _startPos = 0.0f;
    float _currentFeedRate = 1.0f;
    float _maxFeedRate = 1.0f;
    float _minFeedRate = 0.005f;
    float _rapidFeedRate = 1.0f; // Default rapid (retract) feed rate
    float _feedDirection = 1.0f; // Direction of feed (1.0 for forward, -1.0 for reverse)

    // Torque control parameters
    float _torqueTarget = 10.0f;
    float _torquePct = 0.0f;

    // PID control variables
    float _torqueErrorAccumulator = 0.0f;
    uint32_t _lastTorqueUpdateTime = 0;
    static constexpr float TORQUE_Kp = 0.00004f;
    static constexpr float TORQUE_Ki = 0.01f;
    static constexpr float TORQUE_Kd = 0.02f;

    // Moving average buffer for torque smoothing
    static constexpr size_t TORQUE_AVG_BUFFER_SIZE = 64;
    float _torqueBuffer[TORQUE_AVG_BUFFER_SIZE] = { 0.0f };
    uint32_t _torqueTimeBuffer[TORQUE_AVG_BUFFER_SIZE] = { 0 };
    size_t _torqueBufferHead = 0;
    size_t _torqueBufferCount = 0;
    float _smoothedTorque = 0.0f;
    static constexpr uint32_t TORQUE_AVG_WINDOW_MS = 400;

    // Private methods
    void adjustFeedRateBasedOnTorque();
    void startRetract();
    void stopAll();
};
