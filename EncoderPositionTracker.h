// EncoderPositionTracker.h
#pragma once

#include <ClearCore.h>

class EncoderPositionTracker {
public:
    static EncoderPositionTracker& Instance();

    // Initialize with steps per inch calibration values
    void setup(float xStepsPerInch, float yStepsPerInch);
    
    // Call this after homing to establish origin reference
    void resetPositionAfterHoming();
    
    // Call periodically to update absolute position values
    void update();
    
    // Get absolute encoder-verified position (in inches)
    float getAbsolutePositionX() const;
    float getAbsolutePositionY() const;
    
    // Verify current position matches commanded position
    bool verifyPositionX(float expectedInches, float toleranceInches = 0.001f);
    bool verifyPositionY(float expectedInches, float toleranceInches = 0.001f);
    
    // For debugging
    int32_t getRawEncoderCountX() const;
    int32_t getRawEncoderCountY() const;
    
    // Report position error state
    bool hasPositionError() const;
    void clearPositionError();

private:
    EncoderPositionTracker() = default;
    
    float _xStepsPerInch = 0.0f;
    float _yStepsPerInch = 0.0f;
    
    // Absolute encoder counts since homing
    int32_t _encoderCountX = 0;
    int32_t _encoderCountY = 0;
    
    // Previous encoder values to calculate deltas
    int32_t _lastEncoderX = 0;
    int32_t _lastEncoderY = 0;
    
    // Converted positions in inches
    float _absolutePositionXInches = 0.0f;
    float _absolutePositionYInches = 0.0f;
    
    // Position error detection
    bool _positionErrorX = false;
    bool _positionErrorY = false;
    
    // Flag to indicate if position tracking is active
    bool _isInitialized = false;
};
