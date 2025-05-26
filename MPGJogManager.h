// === MPGJogManager.h ===
#pragma once

#include <ClearCore.h>
#include "Config.h"
#include "MotionController.h"

enum AxisId;  // Forward declaration
/// Handles MPG encoder-based jog movements for the selected axis
class MPGJogManager {
public:
    static MPGJogManager& Instance();

    /// Call once in setup()
    void setup();

    /// Call every loop() after MotionController update
    void update();

    /// Called by UIInputManager when encoder turns
    ///  deltaClicks: number of encoder detents (+/-)
    void onEncoderDelta(int deltaClicks);

    /// Select which axis to jog (AXIS_X, AXIS_Y, AXIS_Z)
    void setAxis(AxisId axis);

    /// Set the range multiplier (1, 10, 100)
    void setRangeMultiplier(int multiplier);

    /// Enable or disable MPG-jog handling
    void setEnabled(bool en);
    bool isEnabled() const;
    // Add this to MPGJogManager.h
    float getAxisIncrement() const;

 

private:
    MPGJogManager();

    /// Poll the range selector pins and update range
    void updateRangeFromInputs();

    bool    _initialized = false;
    bool    _enabled = false;
    AxisId  _currentAxis = AXIS_X;
    int     _range = JOG_MULTIPLIER_X1;
};
