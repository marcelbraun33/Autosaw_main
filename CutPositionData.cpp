// Enhanced CutPositionData.cpp
#include "CutPositionData.h"
#include "CutSequenceController.h"
#include "SettingsManager.h"
#include <ClearCore.h>

// Add singleton instance accessor
CutPositionData& CutPositionData::Instance() {
    static CutPositionData instance;
    return instance;
}

void CutPositionData::buildCutSequence() {
    auto& seq = CutSequenceController::Instance();

    // Build X positions based on current parameters
    seq.buildXPositions(_xZeroPosition, _increment, _totalSlices);

    // Set Y parameters
    seq.setYCutStart(_cutStartPosition);
    seq.setYCutStop(_cutEndPosition);
    seq.setYRetract(getRetractPosition());

    ClearCore::ConnectorUsb.Send("[CutPosData] Built sequence with ");
    ClearCore::ConnectorUsb.Send(_totalSlices);
    ClearCore::ConnectorUsb.Send(" cuts, increment: ");
    ClearCore::ConnectorUsb.SendLine(_increment);
}

std::vector<float> CutPositionData::getAllCutPositions() const {
    std::vector<float> positions;
    for (int i = 0; i < _totalSlices; i++) {
        positions.push_back(_xZeroPosition + (i * _increment));
    }
    return positions;
}

bool CutPositionData::isReadyForSequence() const {
    // Check that all required parameters are set
    if (_increment <= 0) {
        ClearCore::ConnectorUsb.SendLine("[CutPosData] Invalid increment");
        return false;
    }
    if (_totalSlices <= 0) {
        ClearCore::ConnectorUsb.SendLine("[CutPosData] Invalid slice count");
        return false;
    }
    if (_cutEndPosition <= _cutStartPosition) {
        ClearCore::ConnectorUsb.SendLine("[CutPosData] Invalid cut positions");
        return false;
    }
    if (_retractDistance <= 0) {
        ClearCore::ConnectorUsb.SendLine("[CutPosData] Invalid retract distance");
        return false;
    }
    return true;
}

float CutPositionData::getCurrentCutProgress() const {
    float cutLength = _cutEndPosition - _cutStartPosition;
    if (cutLength <= 0) return 0.0f;

    float progress = (_currentY - _cutStartPosition) / cutLength;
    if (progress < 0) progress = 0;
    if (progress > 1) progress = 1;
    return progress * 100.0f;  // Return as percentage
}

void CutPositionData::updateCurrentPosition(float x, float y) {
    _currentX = x;
    _currentY = y;

    // Update current slice position based on X
    auto& seq = CutSequenceController::Instance();
    int closestIndex = seq.getPositionIndexForX(x, 0.01f);
    if (closestIndex >= 0) {
        _currentSlicePosition = closestIndex;
    }
}

void CutPositionData::updateFromSettings() {
    auto& settings = SettingsManager::Instance().settings();
    _cutThickness = settings.bladeThickness;
    // Add other settings as needed
}

void CutPositionData::saveToSettings() {
    auto& settings = SettingsManager::Instance().settings();
    settings.bladeThickness = _cutThickness;
    SettingsManager::Instance().save();
}