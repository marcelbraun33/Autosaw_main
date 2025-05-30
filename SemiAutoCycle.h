#pragma once
#include "Cycle.h"
#include "XAxis.h"
#include "YAxis.h"
#include "SettingsManager.h"
#include "cutData.h"

class SemiAutoCycle : public Cycle {
public:
    enum State {
        Idle,
        Ready,
   
        Returning,

        MovingToRetract,
        MovingToStart,
        WaitingAtStart,
        FeedingToStop,
        Retracting,
        Paused,
        Complete,
        Canceled,
        Error
    };

    SemiAutoCycle(CutData& cutData);

    // Set hardware axis pointers (must be called before start)
    void setAxes(YAxis* y);

public:
    // Add this public method
    bool attemptRecovery();

    // Cycle interface
    void start() override;
    void pause() override;
    void resume() override;
    void cancel() override;
    void update() override;

    bool isActive() const override;
    bool isComplete() const override;
    bool isPaused() const override;
    bool hasError() const override;
    const char* errorMessage() const override;

    // UI/Control methods
    void setFeedRate(float rate);
    void setCutPressure(float pressure);
    void setSpindleOn(bool on);
    void moveTableToRetract();
    void moveTableToStart();
    void incrementCutIndex(int delta);
    void feedToStop();

    // State queries for UI
    bool isSpindleOn() const;
    bool isAtRetract() const;
    bool isFeedRateOffsetActive() const;
    bool isCutPressureOffsetActive() const;
    float getCutPressure() const;
    float getFeedRate() const;

    // Data for UI display
    float commandedRPM() const;
    float currentThickness() const;
    float distanceToGo() const;
    int currentCutIndex() const;
    int totalSlices() const;
    State getState() const { return _state; }
    
    // SemiAuto 
    float& getFeedRateRef() { return _feedRate; }
    YAxis* getYAxis() const { return _yAxis; }
    void forceReset();

    // In SemiAutoCycle.h, add:
    float getCutEndPoint() const { return _cutData.cutEndPoint; }
    float& getCutPressureRef() { return _cutPressure; }  // Return reference to cut pressure


private:
    float calculateVelocityScale(float feedRateInchesPerSec) const;
    const char* stateToString(State state) const;

    void transitionTo(State newState);
    void updateStateMachine();

    State _state;
    int _cutIndex;
    bool _spindleOn;
    bool _feedHold;
    float _feedRate;
    float _cutPressure;
    char _errorMsg[64];
    float _feedStartPos = 0.0f;

    XAxis* _xAxis;
    YAxis* _yAxis;
    SettingsManager* _settings;
};
