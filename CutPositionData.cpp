
// CutPositionData.cpp
#include "CutPositionData.h"
#include <math.h>

CutPositionData& CutPositionData::Instance() {
    static CutPositionData instance;
    return instance;
}

CutPositionData::CutPositionData() {
    // Default initialization happens in the header
}

float CutPositionData::getDistanceToGo() const {
    if (_increment <= 0) {
        return 0.0f;
    }

    // Calculate remaining distance based on increment and remaining slices
    int remainingSlices = _totalSlices - _currentSlicePosition;
    if (remainingSlices < 0) remainingSlices = 0;

    return remainingSlices * _increment;
}

float CutPositionData::getNextCutPosition() const {
    // Calculate the X position for the next cut
    return _xZeroPosition + (_currentSlicePosition * _increment);
}
