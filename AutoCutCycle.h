#pragma once
#include "Cycle.h"

class AutoCutCycle : public Cycle {
public:
    AutoCutCycle(CutData& cutData);
    void start() override;
    void pause() override;
    void resume() override;
    void cancel() override;
    void update() override;
    float getFeedRate() const;
    void setFeedRate(float rate);

    bool isActive() const override { return false; }
    bool isComplete() const override { return false; }
    bool isPaused() const override { return false; }
    bool hasError() const override { return false; }
    const char* errorMessage() const override { return nullptr; }

private:
    float _feedRate = 0.0f;
};
