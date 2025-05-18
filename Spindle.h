// Spindle.h
#pragma once

class Spindle {
public:
    Spindle();

    void Setup();
    void Start(float rpm);
    void Stop();
    void SetRPM(float rpm);
    bool IsRunning() const;
    float CommandedRPM() const;
    void EmergencyStop();

private:
    bool running;
    float commandedRPM;
};
