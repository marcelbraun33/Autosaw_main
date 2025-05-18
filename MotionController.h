// === MotionController.h ===
#pragma once

#include "Spindle.h"
#include "XAxis.h"
#include "YAxis.h"
#include "ZAxis.h"

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

    /// Initialize spindle and axes (non-blocking)
    void setup();

    /// Periodic update: call from main loop
    void update();

    //--- Spindle Control ---
    void StartSpindle(float rpm);
    void StopSpindle();
    bool IsSpindleRunning() const;
    float CommandedRPM() const;

    //--- Axis Control ---
    bool moveToWithRate(AxisId axis, float target, float rate);
    float getAxisPosition(AxisId axis) const;
    bool isAxisMoving(AxisId axis) const;
    float getTorquePercent(AxisId axis) const;

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

private:
    MotionController();
    Spindle spindle;
    XAxis xAxis;
    YAxis yAxis;
    ZAxis zAxis;
};
