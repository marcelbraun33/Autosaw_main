#pragma once
#include "Cycle.h"

class AutoCutCycle : public Cycle {
public:
    AutoCutCycle(CutData& cutData) : Cycle(cutData) {}
    void start() override {}
    void pause() override {}
    void resume() override {}
    void cancel() override {}
    void update() override {}

    bool isActive() const override { return false; }
    bool isComplete() const override { return false; }
    bool isPaused() const override { return false; }
    bool hasError() const override { return false; }
    const char* errorMessage() const override { return nullptr; }
};
