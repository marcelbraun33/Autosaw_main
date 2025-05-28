// CutPositionData.h
#pragma once

class CutPositionData {
public:
    // Singleton access
    static CutPositionData& Instance();
    
    // X axis data (from JogXScreen)
    void setXZeroPosition(float pos) { _xZeroPosition = pos; }
    float getXZeroPosition() const { return _xZeroPosition; }
    
    void setStockLength(float length) { _stockLength = length; }
    float getStockLength() const { return _stockLength; }
    
    void setIncrement(float inc) { _increment = inc; }
    float getIncrement() const { return _increment; }
    
    void setCutThickness(float thickness) { _cutThickness = thickness; }
    float getCutThickness() const { return _cutThickness; }
    
    void setTotalSlices(int slices) { _totalSlices = slices; }
    int getTotalSlices() const { return _totalSlices; }
    
    void setCurrentSlicePosition(int pos) { _currentSlicePosition = pos; }
    int getCurrentSlicePosition() const { return _currentSlicePosition; }
    
    // Y axis data (from JogYScreen)
    void setCutStartPosition(float pos) { _cutStartPosition = pos; }
    float getCutStartPosition() const { return _cutStartPosition; }
    
    void setCutEndPosition(float pos) { _cutEndPosition = pos; }
    float getCutEndPosition() const { return _cutEndPosition; }
    
    void setRetractDistance(float dist) { _retractDistance = dist; }
    float getRetractDistance() const { return _retractDistance; }
    
    // Calculate derived values
    float calculateCutLength() const { return _cutEndPosition - _cutStartPosition; }
    float getDistanceToGo() const;
    float getNextCutPosition() const;

private:
    CutPositionData();
    
    // X axis parameters
    float _xZeroPosition = 0.0f;
    float _stockLength = 0.0f;
    float _increment = 0.0f;
    float _cutThickness = 0.0f;
    int _totalSlices = 0;
    int _currentSlicePosition = 0;
    
    // Y axis parameters
    float _cutStartPosition = 0.0f;
    float _cutEndPosition = 0.0f;
    float _retractDistance = 0.5f;
};
