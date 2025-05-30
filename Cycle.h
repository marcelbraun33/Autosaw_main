#pragma once
#include "cutData.h"

class Cycle {
public:
    virtual ~Cycle() {}
    virtual void start() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void cancel() = 0;
    virtual void update() = 0;

    virtual bool isActive() const = 0;
    virtual bool isComplete() const = 0;
    virtual bool isPaused() const = 0;

    // Optional error handling
    virtual bool hasError() const { return false; }
    virtual const char* errorMessage() const { return nullptr; }

protected:
    CutData& _cutData;
    Cycle(CutData& cutData) : _cutData(cutData) {}
};
