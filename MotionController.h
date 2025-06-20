// Updated MotionController.h with torque control methods
#pragma once

#include "Spindle.h"  // Include the Spindle header first
#include "XAxis.h"
#include "YAxis.h"
#include "ZAxis.h"
#include "Config.h"

/// Identifiers for each axis
enum AxisId {
    AXIS_X = 0,
    AXIS_Y = 1,
    AXIS_Z = 2
};

/// Central motion controller for spindle and axes
class MotionController {
public:
    /// Singleton access
    static MotionController& Instance();
    void ClearAxisAlerts();

    /// Initialize spindle and axes (non-blocking)
    void setup();

    /// Periodic update: call from main loop
    void update();

    //--- Spindle Control ---
    void StartSpindle(float rpm);
    void StopSpindle();
    bool IsSpindleRunning() const;
    float CommandedRPM() const;
    /// Get the spindle load percentage from the HLFB (High-Level Feedback)
    float getSpindleLoadPercent() const;

    //--- Axis Control ---
    bool moveToWithRate(AxisId axis, float target, float rate);
    float getAxisPosition(AxisId axis) const;
    bool isAxisMoving(AxisId axis) const;
    float getTorquePercent(AxisId axis) const;
    /// Move the given axis to an absolute position (inches), at the given speed scale (0..1)
    bool moveTo(AxisId axis, float positionInches, float velocityScale);

    /// Jog the given axis by a relative offset (inches), at the given speed scale (0..1)
    bool jogBy(AxisId axis, float deltaInches, float velocityScale);

    //--- Absolute Position Tracking ---
    /// Get absolute encoder-verified position for an axis
    float getAbsoluteAxisPosition(AxisId axis) const;

    /// Verify that commanded position matches actual encoder position
    bool verifyAxisPosition(AxisId axis, float expectedInches, float toleranceInches = ENCODER_POSITION_TOLERANCE);

    /// Check if encoder position tracking has detected an error
    bool hasEncoderPositionError() const;

    /// Clear any encoder position errors
    void clearEncoderPositionErrors();

    //--- Torque-Controlled Feed ---
    /// Start a torque-controlled feed for Y-axis to maintain constant cutting force
    bool startTorqueControlledFeed(AxisId axis, float targetPosition, float initialVelocityScale);

    // Optionally, add a convenience inline overload in the header:
    bool startTorqueControlledFeed(AxisId axis, float targetPosition) {
        return startTorqueControlledFeed(axis, targetPosition, 1.0f);
    }

    /// Set the torque target for the specified axis
    void setTorqueTarget(AxisId axis, float targetPercent);

    /// Get the current torque target for the specified axis
    float getTorqueTarget(AxisId axis) const;

    /// Check if the specified axis is in torque-controlled feed mode
    bool isInTorqueControlledFeed(AxisId axis) const;

    /// Abort a torque-controlled feed operation
    void abortTorqueControlledFeed(AxisId axis);

    // Ensure these methods are public and implemented:
    void pauseTorqueControlledFeed(int axis);
    void resumeTorqueControlledFeed(int axis);
    void abortTorqueControlledFeed(int axis);
    void moveTo(int axis, float position, float velocityScale = 1.0f);

    // Add these methods to match usage in AutoCutCycleManager
    bool isAxisAtPosition(int axis, float position) const;
    bool isFeedComplete() const;
    float getSpindleRPM() const;
    void MoveAxisTo(int axis, float position, float velocityScale = 1.0f);

    //--- Emergency ---
    void EmergencyStop();

    //--- Status ---
    struct MotionStatus {
        bool spindleRunning;
        float spindleRPM;
        float xPosition, yPosition, zPosition;
        bool xMoving, yMoving, zMoving;
        bool xHomed, yHomed, zHomed;
    };
    MotionStatus getStatus() const;
    // In public section:
    bool StartHomingAxis(AxisId axis);
    bool StartHomingAll();  // optional convenience for full-machine homing

    // Add this accessor to expose the YAxis instance for feed rate monitoring
    YAxis &YAxisInstance();

private:
    MotionController();
    Spindle spindle;
    XAxis xAxis;
    YAxis yAxis;
    ZAxis zAxis;
};

