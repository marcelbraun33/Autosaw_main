#pragma once

class CutSequenceController {
public:
    static CutSequenceController& Instance();
    void setup();
    void update();
    void startCycle();
    void cancelCycle();
    bool isRunning() const;

private:
    CutSequenceController() = default;
    bool _running = false;
};
