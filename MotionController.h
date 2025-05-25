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

    //--- Axis Control ---
    bool moveToWithRate(AxisId axis, float target, float rate);
    float getAxisPosition(AxisId axis) const;
    bool isAxisMoving(AxisId axis) const;
    float getTorquePercent(AxisId axis) const;
    /// Move the given axis to an absolute position (inches), at the given speed scale (0..1)
    bool moveTo(AxisId axis, float positionInches, float velocityScale);

    /// Jog the given axis by a relative offset (inches), at the given speed scale (0..1)
    bool jogBy(AxisId axis, float deltaInches, float velocityScale);


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


private:
    MotionController();
    Spindle spindle;
    XAxis xAxis;
    YAxis yAxis;
    ZAxis zAxis;
};