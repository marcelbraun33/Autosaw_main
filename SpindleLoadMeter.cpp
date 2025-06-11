#include "SpindleLoadMeter.h"
#include "MotionController.h"
#include <ClearCore.h>
#include <genieArduinoDEV.h> 

extern Genie genie;  // Add semicolon here

SpindleLoadMeter::SpindleLoadMeter(uint16_t gaugeIndex)
    : _gaugeIndex(gaugeIndex) {
}

void SpindleLoadMeter::Update() {
    if (!MotionController::Instance().IsSpindleRunning()) {
        genie.WriteObject(GENIE_OBJ_IGAUGE, _gaugeIndex, 0);
        return;
    }

    float duty = MOTOR_SPINDLE.HlfbPercent();
    if (duty > 1.0f) duty = duty / 100.0f;

    const float alpha = 0.005f;
    _filteredDuty = alpha * duty + (1.0f - alpha) * _filteredDuty;

    int16_t displayValue = static_cast<int16_t>(_filteredDuty * 100.0f + 0.5f);
    genie.WriteObject(GENIE_OBJ_IGAUGE, _gaugeIndex, displayValue);

    uint32_t currentTime = ClearCore::TimingMgr.Milliseconds();
    if (currentTime - _lastLogTime > 1000) {
        _lastLogTime = currentTime;
        ClearCore::ConnectorUsb.Send("[SpindleLoad] Raw duty: ");
        ClearCore::ConnectorUsb.Send(duty * 100.0f, 0);
        ClearCore::ConnectorUsb.Send("%, Filtered: ");
        ClearCore::ConnectorUsb.SendLine(displayValue);
    }
}
