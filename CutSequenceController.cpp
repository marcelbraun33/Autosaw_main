#include "CutSequenceController.h"

CutSequenceController& CutSequenceController::Instance() {
    static CutSequenceController instance;
    return instance;
}

CutSequenceController::CutSequenceController() {}

void CutSequenceController::setXIncrements(const std::vector<float>& increments) {
    _xIncrements = increments;
    _currentIndex = 0;
}
void CutSequenceController::setYCutStart(float yStart) { _yCutStart = yStart; }
void CutSequenceController::setYCutStop(float yStop) { _yCutStop = yStop; }
void CutSequenceController::setYRetract(float yRetract) { _yRetract = yRetract; }

float CutSequenceController::getCurrentX() const {
    if (_currentIndex < _xIncrements.size()) return _xIncrements[_currentIndex];
    return 0.0f;
}
float CutSequenceController::getNextX() const {
    if (_currentIndex + 1 < _xIncrements.size()) return _xIncrements[_currentIndex + 1];
    return 0.0f;
}
float CutSequenceController::getYCutStart() const { return _yCutStart; }
float CutSequenceController::getYCutStop() const { return _yCutStop; }
float CutSequenceController::getYRetract() const { return _yRetract; }

void CutSequenceController::reset() { _currentIndex = 0; }
bool CutSequenceController::advance() {
    if (_currentIndex + 1 < _xIncrements.size()) {
        ++_currentIndex;
        return true;
    }
    return false;
}
bool CutSequenceController::isComplete() const {
    return _currentIndex >= _xIncrements.size();
}
int CutSequenceController::getCurrentIndex() const { return _currentIndex; }
int CutSequenceController::getTotalCuts() const { return static_cast<int>(_xIncrements.size()); }

// --- New X position tracking and comparison methods ---

void CutSequenceController::setCurrentXPosition(float x) {
    _currentXPosition = x;
}

float CutSequenceController::getCurrentXPosition() const {
    return _currentXPosition;
}

bool CutSequenceController::isAtCurrentIncrement(float tolerance) const {
    if (_currentIndex < _xIncrements.size()) {
        return std::fabs(_currentXPosition - _xIncrements[_currentIndex]) < tolerance;
    }
    return false;
}

int CutSequenceController::findClosestIncrementIndex(float x, float tolerance) const {
    for (size_t i = 0; i < _xIncrements.size(); ++i) {
        if (std::fabs(x - _xIncrements[i]) < tolerance) {
            return static_cast<int>(i);
        }
    }
    return -1; // Not found
}
void CutSequenceController::buildXPositions(float stockZero, float increment, int totalSlices) {
    _xIncrements.clear();
    for (int i = 0; i < totalSlices; ++i) {
        _xIncrements.push_back(stockZero + i * increment);
    }
    _currentIndex = 0;
}

int CutSequenceController::getClosestIndexForPosition(float x, float tolerance) const {
    int closest = -1;
    float minDiff = tolerance;
    for (size_t i = 0; i < _xIncrements.size(); ++i) {
        float diff = std::fabs(x - _xIncrements[i]);
        if (diff < minDiff) {
            minDiff = diff;
            closest = static_cast<int>(i);
        }
    }
    return closest;
}

int CutSequenceController::getPositionIndexForX(float x, float tolerance) const {
    int closest = -1;
    float minDiff = tolerance;
    for (size_t i = 0; i < _xIncrements.size(); ++i) {
        float diff = std::fabs(x - _xIncrements[i]);
        if (diff < minDiff) {
            minDiff = diff;
            closest = static_cast<int>(i);
        }
    }
    // If not found, return 0 (stock zero position)
    return (closest >= 0) ? closest : 0;
}

float CutSequenceController::getXForIndex(int idx) const {
    if (idx >= 0 && idx < static_cast<int>(_xIncrements.size())) {
        return _xIncrements[idx];
    }
    return 0.0f;
}
