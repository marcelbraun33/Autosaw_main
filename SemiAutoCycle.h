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

    // Constructor: initializes the underlying CutData reference
    SemiAutoCycle(CutData& cutData);
    bool _hasStartedMove = false;


    // Must set Y axis pointer before calling start()
    void setAxes(YAxis* y);

    // Core Cycle interface
    void start() override;
    void pause() override;
    void resume() override;
    void cancel() override;
    void update() override;

    bool isActive()   const override;
    bool isComplete() const override;
    bool isPaused()   const override;
    bool hasError()   const override;
    const char* errorMessage() const override;

    // Recovery and reset
    bool attemptRecovery();
    void forceReset();

    // Live-feed adjustments
    void setFeedRate(float rate);
    void setCutPressure(float pressure);
    void updateFeedRateVelocity();

    // Spindle and table commands
    void setSpindleOn(bool on);
    void moveTableToRetract();
    void moveTableToStart();
    void incrementCutIndex(int delta);
    void feedToStop();

    // State & Data Queries for UI
    State getState()                      const { return _state; }
    bool  isSpindleOn()                   const;
    bool  isAtRetract()                   const;
    bool  isFeedRateOffsetActive()        const;
    bool  isCutPressureOffsetActive()     const;
    float getFeedRate()                   const { return _feedRate; }
    float getCutPressure()                const { return _cutPressure; }
    float commandedRPM()                  const;
    float currentThickness()              const;
    float distanceToGo()                  const;
    float getDisplayDistanceToGo()        const { return _displayDist; }
    float getCutEndPoint()                const { return _cutData.cutEndPoint; }
    int   currentCutIndex()               const { return _cutIndex; }
    int   totalSlices()                   const { return _cutData.totalSlices; }

    // References for UI binding
    float& getFeedRateRef() { return _feedRate; }
    float& getCutPressureRef() { return _cutPressure; }

    // Axis access for screens
    YAxis* getYAxis()                     const { return _yAxis; }

private:
    // Internal state machine helpers
    void transitionTo(State newState);
    void updateStateMachine();
    const char* stateToString(State state) const;
    float calculateVelocityScale(float feedRateInchesPerSec) const;

    // Internal fields
    State    _state = Idle;
    int      _cutIndex = 0;
    bool     _spindleOn = false;
    bool     _feedHold = false;
    float    _feedRate = 0.0f;
    float    _cutPressure = 0.0f;
    char     _errorMsg[64] = { 0 };
    float    _feedStartPos = 0.0f;
    float    _displayDist = 0.0f;

    XAxis* _xAxis = nullptr;
    YAxis* _yAxis = nullptr;
    SettingsManager* _settings = nullptr;

    CutData& _cutData;
};
