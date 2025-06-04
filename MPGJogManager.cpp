// MPGJogManager.cpp
#include "MPGJogManager.h"
#include "MotionController.h"
#include "Config.h"
#include <ClearCore.h>

MPGJogManager& MPGJogManager::Instance() {
    static MPGJogManager inst;
    return inst;
}

MPGJogManager::MPGJogManager() = default;

void MPGJogManager::setup() {
    _initialized = true;

    // Configure range selector pins for input
    RANGE_PIN_X10.Mode(Connector::INPUT_DIGITAL);
    RANGE_PIN_X100.Mode(Connector::INPUT_DIGITAL);

    // For ClearCore, you can't directly set pull-ups on analog inputs
    // If needed, configure pull-ups in hardware or use INPUT_DIGITAL_PULLUP if available
    
    // Initialize range (don't assume X1)
    updateRangeFromInputs();

    ClearCore::ConnectorUsb.Send("[MPG] Setup complete, range set to X");
    ClearCore::ConnectorUsb.SendLine(_range);
}

void MPGJogManager::setEnabled(bool en) {
    _enabled = en;
    if (en) {
        ClearCore::ConnectorUsb.SendLine("[MPG] Enabled");
    } else {
        ClearCore::ConnectorUsb.SendLine("[MPG] Disabled");
    }
}

bool MPGJogManager::isEnabled() const {
    return _enabled;
}

void MPGJogManager::setAxis(AxisId axis) {
    _currentAxis = axis;
    const char* axisName = "";
    switch (axis) {
    case AXIS_X: axisName = "X"; break;
    case AXIS_Y: axisName = "Y"; break;
    case AXIS_Z: axisName = "Z"; break;
    }
    ClearCore::ConnectorUsb.Send("[MPG] Active axis changed to ");
    ClearCore::ConnectorUsb.SendLine(axisName);
}

void MPGJogManager::setRangeMultiplier(int multiplier) {
    // Update the range multiplier and log the change
    if (multiplier == JOG_MULTIPLIER_X1 || 
        multiplier == JOG_MULTIPLIER_X10 || 
        multiplier == JOG_MULTIPLIER_X100) {
        _range = multiplier;
        
      // ClearCore::ConnectorUsb.Send("[MPG] Range set to X");
      //  ClearCore::ConnectorUsb.SendLine(_range);
    } else {
        ClearCore::ConnectorUsb.Send("[MPG] Invalid range multiplier: ");
        ClearCore::ConnectorUsb.SendLine(multiplier);
    }
}

void MPGJogManager::updateRangeFromInputs() {
    // Read the range selector pins
    bool x10Selected = RANGE_PIN_X10.State();
    bool x100Selected = RANGE_PIN_X100.State();
    
    // Debug pin states
    ClearCore::ConnectorUsb.Send("[MPG] Pin states: X10=");
    ClearCore::ConnectorUsb.Send(x10Selected ? "HIGH" : "LOW");
    ClearCore::ConnectorUsb.Send(", X100=");
    ClearCore::ConnectorUsb.SendLine(x100Selected ? "HIGH" : "LOW");
    
    int newRange;
    if (x100Selected) {
        newRange = JOG_MULTIPLIER_X100;
    } else if (x10Selected) {
        newRange = JOG_MULTIPLIER_X10;
    } else {
        newRange = JOG_MULTIPLIER_X1;
    }
    
    // Only update and log if the range has changed
    if (newRange != _range) {
        _range = newRange;
       // ClearCore::ConnectorUsb.Send("[MPG] Range changed to X");
       // ClearCore::ConnectorUsb.SendLine(_range);
    }
}
float MPGJogManager::getAxisIncrement() const {
    // Return the current increment based on range
    switch (_range) {
    case JOG_MULTIPLIER_X1:   return 0.0003f;
    case JOG_MULTIPLIER_X10:  return 0.001f;
    case JOG_MULTIPLIER_X100: return 0.01f;
    default:                  return 0.0003f;
    }
}


// MPGJogManager.cpp - update the onEncoderDelta method

// Update MPGJogManager.cpp
// Update onEncoderDelta method to use absolute positions from encoder

void MPGJogManager::onEncoderDelta(int deltaClicks) {
    if (!_initialized || !_enabled || deltaClicks == 0)
        return;

    // inches per click for each range
    float stepInches;
    switch (_range) {
    case JOG_MULTIPLIER_X1:
        // Increase the step size to ensure at least 1 motor step
        stepInches = 0.0003f;  // Try a larger value to ensure movement
        break;
    case JOG_MULTIPLIER_X10:  stepInches = 0.0010f; break;
    case JOG_MULTIPLIER_X100: stepInches = 0.0100f; break;
    default:                  stepInches = 0.0003f; break;
    }

    // total move distance
    float inches = deltaClicks * stepInches;

    // Scale velocity with range - slower for X1 to ensure precision
    float velocityScale;
    switch (_range) {
    case JOG_MULTIPLIER_X1:   velocityScale = 0.15f; break; // Reduced to 15% for finer control
    case JOG_MULTIPLIER_X10:  velocityScale = 0.5f; break;
    case JOG_MULTIPLIER_X100: velocityScale = 0.9f; break;
    default:                  velocityScale = 0.5f; break;
    }

    // Only log meaningful movement for debugging
    if (abs(deltaClicks) > 0) {
        ClearCore::ConnectorUsb.Send("[MPG] Moving ");
        ClearCore::ConnectorUsb.Send(inches, 6);
        ClearCore::ConnectorUsb.Send(" inches at ");
        ClearCore::ConnectorUsb.Send(velocityScale * 100.0f, 0);
        ClearCore::ConnectorUsb.Send("% speed, range=X");
        ClearCore::ConnectorUsb.Send(_range);
        ClearCore::ConnectorUsb.Send(", delta=");
        ClearCore::ConnectorUsb.SendLine(deltaClicks);
    }

    // Use absolute position from encoder tracker
    MotionController::Instance().moveToWithRate(
        _currentAxis,
        MotionController::Instance().getAbsoluteAxisPosition(_currentAxis) + inches,
        velocityScale
    );
}



void MPGJogManager::update() {
    // Only update the range from inputs if necessary
    // Reduce frequency to avoid excessive polling
    static uint32_t lastRangeCheckTime = 0;
    uint32_t now = ClearCore::TimingMgr.Milliseconds();

    if (now - lastRangeCheckTime > 250) {  // Check every 250ms
        updateRangeFromInputs();
        lastRangeCheckTime = now;
    }
}
