#pragma once

#include <stdint.h>

class SpindleLoadMeter {
public:
    SpindleLoadMeter(uint16_t gaugeIndex);
    void Update();

private:
    uint16_t _gaugeIndex;
    float _filteredDuty = 0.0f;
    uint32_t _lastLogTime = 0;
};
