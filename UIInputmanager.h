// UIInputManager.h
#pragma once

#include <genieArduinoDEV.h>
#include <ClearCore.h>
#include <cstdint>
#include <cmath>

class UIInputManager {
public:
    static UIInputManager& Instance();

    // Call once in setup()
    void init(int countsPerClick = 4);

    // Bind a WinButton ? LEDDigits field to your MPG encoder
    void bindField(uint8_t winButtonId,
        uint8_t ledDigitId,
        float* storagePtr,
        float min,
        float max,
        float step,
        uint8_t decimalPlaces);

    // Unbind when the user confirms or cancels
    void unbindField();

    // Call every loop() to process encoder movement
    void update();

    bool isEditing() const { return binding.active; }

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
};
