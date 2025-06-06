#pragma once

#include <ClearCore.h>

class YAxis; // Forward declaration

/**
 * DynamicFeed module - Handles torque-controlled feed operations for the Y axis
 * 
 * This class encapsulates logic for torque feedback-based feed rate control,
 * primarily used during cutting operations where constant torque is desirable.
 */
class DynamicFeed {
public:
    /**
     * Constructor
     * 
     * @param owner Reference to the YAxis that owns this DynamicFeed instance
     * @param stepsPerInch Conversion factor for steps to inches
     * @param motor Pointer to the associated ClearCore motor driver
     */
    DynamicFeed(YAxis* owner, float stepsPerInch, MotorDriver* motor);
    
    /**
     * Destructor
     */
    ~DynamicFeed();
    
    /**
     * Start a torque-controlled feed operation
     * 
     * @param targetPosition Target position in inches
     * @param initialVelocityScale Initial velocity as a scale factor (0.0-1.0)
     * @return true if feed operation started successfully
     */
    bool start(float targetPosition, float initialVelocityScale);
    
    /**
     * Abort an in-progress torque-controlled feed operation
     */
    void abort();
    
    /**
     * Update the dynamic feed controller
     * Must be called regularly during operation
     * 
     * @param currentPos The current Y axis position
     * @param isMoving Whether the Y axis is currently moving
     * @return true if target position reached
     */
    bool update(float currentPos);
    
    /**
     * Check if a torque-controlled feed operation is active
     * 
     * @return true if a feed operation is active
     */
    bool isActive() const;
    
    /**
     * Set the target torque for the feed controller
     * 
     * @param targetPercent Target torque percentage (1.0-95.0)
     */
    void setTorqueTarget(float targetPercent);
    
    /**
     * Get the current torque target value
     * 
     * @return Current torque target percentage
     */
    float getTorqueTarget() const;
    
    /**
     * Update the current feed rate directly
     * 
     * @param newVelocityScale New velocity scale (0.0-1.0)
     */
    void updateFeedRate(float newVelocityScale);
    
    /**
     * Update the torque measurement from the motor
     * This must be called regularly to maintain accurate torque readings
     * 
     * @return The updated torque reading
     */
    float updateTorqueMeasurement();
    
    /**
     * Get the current measured torque value
     * 
     * @return Current torque percentage (filtered)
     */
    float getTorquePercent() const;
    
    /**
     * Get the current feed rate
     * 
     * @return Current feed rate as a scale factor (0.0-1.0)
     */
    float getCurrentFeedRate() const;

private:
    YAxis* _owner;                     // Pointer to owning YAxis
    MotorDriver* const _motor;         // Pointer to ClearCore motor driver
    const float _stepsPerInch;         // Conversion factor for steps to inches

    // Feed state
    bool _isActive = false;            // True when feed is active
    float _targetPos = 0.0f;           // Target position in inches 
    float _currentFeedRate = 1.0f;     // Current feed rate (0.0-1.0)
    float _maxFeedRate = 1.0f;         // Maximum feed rate for torque control
    float _minFeedRate = 0.005f;       // Minimum feed rate

    // Torque control parameters
    float _torqueTarget = 10.0f;       // Default target torque (%)
    float _torquePct = 0.0f;           // Current torque reading
    
    // PID control variables
    float _torqueErrorAccumulator = 0.0f;
    uint32_t _lastTorqueUpdateTime = 0;
    static constexpr float TORQUE_Kp = 0.00004f;  // Proportional gain
    static constexpr float TORQUE_Ki = 0.01f;     // Integral gain 
    static constexpr float TORQUE_Kd = 0.02f;     // Derivative gain

    // Moving average buffer for torque smoothing
    static constexpr size_t TORQUE_AVG_BUFFER_SIZE = 64;  // Enough for 400ms at ~6ms/sample
    float _torqueBuffer[TORQUE_AVG_BUFFER_SIZE] = {0.0f};
    uint32_t _torqueTimeBuffer[TORQUE_AVG_BUFFER_SIZE] = {0};
    size_t _torqueBufferHead = 0;
    size_t _torqueBufferCount = 0;
    float _smoothedTorque = 0.0f;
    static constexpr uint32_t TORQUE_AVG_WINDOW_MS = 400;

    // Private methods
    void adjustFeedRateBasedOnTorque();
};
