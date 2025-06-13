#pragma once

#include "ICycle.h"
#include "Spindle.h"

/// Cycle to start or stop the spindle
class StartSpindleCycle : public ICycle {
public:
    // state: true = start spindle, false = stop spindle
    StartSpindleCycle(Spindle &spindle, bool state);

    void start() override;
    void stop() override;
    bool isDone() const override;
    bool hasError() const override;
    CycleError error() const override;

private:
    Spindle &_spindle;
    bool _targetState;
    bool _started = false;
    bool _done = false;
    bool _error = false;
    CycleError _errorCode = CycleError::None;
};
