// EncoderPositionTracker.cpp
#include "EncoderPositionTracker.h"
#include "Config.h"
#include <ClearCore.h>

EncoderPositionTracker& EncoderPositionTracker::Instance() {
    static EncoderPositionTracker instance;
    return instance;
}

void EncoderPositionTracker::setup(float xStepsPerInch, float yStepsPerInch) {
    _xStepsPerInch = xStepsPerInch;
    _yStepsPerInch = yStepsPerInch;
    
    // Initialize encoder reading from motors
    _lastEncoderX = MOTOR_FENCE_X.PositionRefCommanded();
    _lastEncoderY = MOTOR_SAW_Y.PositionRefCommanded();
    
    _encoderCountX = 0;
    _encoderCountY = 0;
    _absolutePositionXInches = 0.0f;
    _absolutePositionYInches = 0.0f;
    _positionErrorX = false;
    _positionErrorY = false;
    
    _isInitialized = true;
    
    ClearCore::ConnectorUsb.SendLine("[EncoderTracker] Setup complete");
}

void EncoderPositionTracker::resetPositionAfterHoming() {
    _encoderCountX = 0;
    _encoderCountY = 0;
    
    // Get current encoder values after homing
    _lastEncoderX = MOTOR_FENCE_X.PositionRefCommanded();
    _lastEncoderY = MOTOR_SAW_Y.PositionRefCommanded();
    
    _absolutePositionXInches = 0.0f;
    _absolutePositionYInches = 0.0f;
    _positionErrorX = false;
    _positionErrorY = false;
    
    ClearCore::ConnectorUsb.SendLine("[EncoderTracker] Position reset after homing");
}

void EncoderPositionTracker::update() {
    if (!_isInitialized) return;
    
    // Read current encoder values
    int32_t currentEncoderX = MOTOR_FENCE_X.PositionRefCommanded();
    int32_t currentEncoderY = MOTOR_SAW_Y.PositionRefCommanded();
    
    // Calculate delta movements
    int32_t deltaX = currentEncoderX - _lastEncoderX;
    int32_t deltaY = currentEncoderY - _lastEncoderY;
    
    // Update accumulated counts
    _encoderCountX += deltaX;
    _encoderCountY += deltaY;
    
    // Update last values for next iteration
    _lastEncoderX = currentEncoderX;
    _lastEncoderY = currentEncoderY;
    
    // Convert to inches
    _absolutePositionXInches = static_cast<float>(_encoderCountX) / _xStepsPerInch;
    _absolutePositionYInches = static_cast<float>(_encoderCountY) / _yStepsPerInch;
    
    // Log significant position changes (reduce noise)
    static uint32_t lastLogTime = 0;
    uint32_t now = ClearCore::TimingMgr.Milliseconds();
    if (now - lastLogTime > 1000) { // Log every second
        lastLogTime = now;
        ClearCore::ConnectorUsb.Send("[EncoderTracker] X: ");
        ClearCore::ConnectorUsb.Send(_absolutePositionXInches, 4);
        ClearCore::ConnectorUsb.Send(" inches, Y: ");
        ClearCore::ConnectorUsb.SendLine(_absolutePositionYInches, 4);
    }
}

float EncoderPositionTracker::getAbsolutePositionX() const {
    return _absolutePositionXInches;
}

float EncoderPositionTracker::getAbsolutePositionY() const {
    return _absolutePositionYInches;
}

bool EncoderPositionTracker::verifyPositionX(float expectedInches, float toleranceInches) {
    bool isValid = fabs(_absolutePositionXInches - expectedInches) <= toleranceInches;
    
    if (!isValid && !_positionErrorX) {
        _positionErrorX = true;
        ClearCore::ConnectorUsb.Send("[EncoderTracker] ERROR: X position mismatch. Expected: ");
        ClearCore::ConnectorUsb.Send(expectedInches, 4);
        ClearCore::ConnectorUsb.Send(", Actual: ");
        ClearCore::ConnectorUsb.Send(_absolutePositionXInches, 4);
        ClearCore::ConnectorUsb.Send(", Diff: ");
        ClearCore::ConnectorUsb.SendLine(fabs(_absolutePositionXInches - expectedInches), 4);
    }
    
    return isValid;
}

bool EncoderPositionTracker::verifyPositionY(float expectedInches, float toleranceInches) {
    bool isValid = fabs(_absolutePositionYInches - expectedInches) <= toleranceInches;
    
    if (!isValid && !_positionErrorY) {
        _positionErrorY = true;
        ClearCore::ConnectorUsb.Send("[EncoderTracker] ERROR: Y position mismatch. Expected: ");
        ClearCore::ConnectorUsb.Send(expectedInches, 4);
        ClearCore::ConnectorUsb.Send(", Actual: ");
        ClearCore::ConnectorUsb.Send(_absolutePositionYInches, 4);
        ClearCore::ConnectorUsb.Send(", Diff: ");
        ClearCore::ConnectorUsb.SendLine(fabs(_absolutePositionYInches - expectedInches), 4);
    }
    
    return isValid;
}

int32_t EncoderPositionTracker::getRawEncoderCountX() const {
    return _encoderCountX;
}

int32_t EncoderPositionTracker::getRawEncoderCountY() const {
    return _encoderCountY;
}

bool EncoderPositionTracker::hasPositionError() const {
    return _positionErrorX || _positionErrorY;
}

void EncoderPositionTracker::clearPositionError() {
    _positionErrorX = false;
    _positionErrorY = false;
    ClearCore::ConnectorUsb.SendLine("[EncoderTracker] Position errors cleared");
}
