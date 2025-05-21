// core/EStopManager.h
#pragma once
#include <ClearCore.h>

class EStopManager {
public:
    static EStopManager& Instance();

    /// Call once from AutoSawController::setup()
    void setup();

    /// Call once per loop, before any motion
    void update();

    bool isActivated() const;
    bool isSafetyRelayEnabled() const;

    /// If you want to defer auto-reset until later
    void requestReset();
    void setAutoReset(bool on);

private:
    EStopManager();
    void emergencyStop();

    bool _relayEnabled;
    bool _resetRequested;
    bool _prevSafeState;
    bool _autoReset;
};
