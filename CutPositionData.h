// Enhanced CutPositionData.h - Add these methods to your existing class
#pragma once
#include <vector>

class CutPositionData {
public:
    // === EXISTING METHODS (keep all of these) ===
    static CutPositionData& Instance();

    // X axis data
    void setXZeroPosition(float pos) { _xZeroPosition = pos; }
    float getXZeroPosition() const { return _xZeroPosition; }
    // ... rest of existing methods ...

    // === NEW METHODS FOR SEQUENCE INTEGRATION ===

    // Build cut positions and update the sequence controller
    void buildCutSequence();

    // Get all X positions for cuts
    std::vector<float> getAllCutPositions() const;

    // Validate that all required parameters are set
    bool isReadyForSequence() const;

    // Get retract position (absolute Y coordinate)
    float getRetractPosition() const {
        return _cutStartPosition - _retractDistance;
    }

    // Progress tracking helpers
    float getCurrentCutProgress() const;
    void updateCurrentPosition(float x, float y);

    // Settings integration
    void updateFromSettings();
    void saveToSettings();

private:
    // === EXISTING MEMBERS (unchanged) ===
    float _xZeroPosition = 0.0f;
    float _stockLength = 0.0f;
    float _increment = 0.0f;
    float _cutThickness = 0.0f;
    int _totalSlices = 0;
    int _currentSlicePosition = 0;
    float _cutStartPosition = 0.0f;
    float _cutEndPosition = 0.0f;
    float _retractDistance = 0.5f;

    // === NEW MEMBERS ===
    float _currentX = 0.0f;
    float _currentY = 0.0f;
};