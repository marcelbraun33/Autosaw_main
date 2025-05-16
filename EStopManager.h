#pragma once
#include <ClearCore.h>

class EStopManager {
public:
    static EStopManager& Instance();

    void setup();
    void update();

    // Check if E-Stop is currently activated
    bool isActivated() const;

    // Check if safety relay is enabled
    bool isSafetyRelayEnabled() const;

    // Request system reset after E-Stop condition cleared
    void requestReset();
    
    // Configure auto-reset behavior
    void setAutoReset(bool enabled);
    bool isAutoResetEnabled() const;

private:
    EStopManager();

    void emergencyStop();

    bool safetyRelayEnabled;
    bool resetRequested;
    bool prevEStopState;       // Previous E-Stop state for edge detection
    bool autoResetEnabled;     // Whether to auto-reset when E-Stop is released
};
