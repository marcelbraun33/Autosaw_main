
#pragma once

class MotionController {
public:
    static MotionController& Instance();

    void setup();
    void update();

    void StartSpindle(float rpm);
    void StopSpindle();
    void SetManualOverrideRPM(float rpm);

    // Add emergency stop method
    void EmergencyStop();

    bool IsSpindleRunning() const;
    float CommandedRPM() const;

private:
    MotionController();

    bool spindleRunning = false;
    float commandedRPM = 0.0f;
    float currentRPM = 0.0f;        // Actual RPM during ramping
    unsigned long lastUpdateTime = 0;  // Time tracking for smooth ramping
};
