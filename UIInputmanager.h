// UIInputManager.h
#pragma once

#include "Config.h"          // for ENCODER_COUNTS_PER_CLICK
#include <genieArduinoDEV.h>
#include <ClearCore.h>
#include <cstdint>
#include <cmath>

/// Manages both field‐editing and MPG jog delegation
class UIInputManager {
public:
    static UIInputManager& Instance();

    /// Call once in setup()
    void init(int countsPerClick = ENCODER_COUNTS_PER_CLICK);

    /// Bind a WinButton+LEDDigits pair for numeric editing
    void bindField(uint8_t winButtonId,
        uint8_t ledDigitId,
        float* storagePtr,
        float min,
        float max,
        float step,
        uint8_t decimalPlaces);

    /// Unbind the current field (enter/exit editing)
    void unbindField();

    /// Returns true if a field is active (so we can filter events)
    bool isFieldActive(uint16_t buttonId);

    /// Returns true if currently editing a field
    bool isEditing() const;

    /// Call every loop() to process encoder—either MPG jog or field edit
    void update();

    /// Reset the raw encoder baseline (so future delta starts from zero)
    void resetRaw();

private:
    UIInputManager();
    int countsPerClick;

    struct {
        uint8_t  winButtonId;
        uint8_t  ledDigitId;
        float* valuePtr;
        float    min, max, step;
        uint8_t  decimalPlaces;
        int32_t  lastDetent;
        bool     active;
    } binding;

    /// Last‐seen raw encoder count, for jog mode
    int32_t  _lastRaw;
};
