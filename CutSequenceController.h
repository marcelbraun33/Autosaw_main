// CutSequenceController.h
#pragma once
#include <vector>
#include <cmath>

class CutSequenceController {
public:
    static CutSequenceController& Instance();

    // Sequence management
    void setXIncrements(const std::vector<float>& increments);
    void setYCutStart(float yStart);
    void setYCutStop(float yStop);
    void setYRetract(float yRetract);

    float getCurrentX() const;
    float getNextX() const;
    float getYCutStart() const;
    float getYCutStop() const;
    float getYRetract() const;

    void reset();
    bool advance();
    bool isComplete() const;
    int getCurrentIndex() const;
    int getTotalCuts() const;

    void buildXPositions(float stockZero, float increment, int totalSlices);
    int getClosestIndexForPosition(float x, float tolerance = 0.005f) const;

    // X position tracking and comparison
    void setCurrentXPosition(float x);
    float getCurrentXPosition() const;
    bool isAtCurrentIncrement(float tolerance = 0.001f) const;
    int findClosestIncrementIndex(float x, float tolerance = 0.001f) const;

    // Returns the 0-based index of the increment closest to the given X position
    int getPositionIndexForX(float x, float tolerance = 0.001f) const;

    // Returns the X position for the given index (or 0.0f if out of range)
    float getXForIndex(int idx) const;

private:
    CutSequenceController();
    std::vector<float> _xIncrements;
    float _yCutStart = 0.0f;
    float _yCutStop = 0.0f;
    float _yRetract = 0.0f;
    int _currentIndex = 0;

    // Track the current X position
    float _currentXPosition = 0.0f;
};
