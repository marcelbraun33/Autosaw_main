// UIInputManager.cpp
#include "UIInputManager.h"
#include <genieArduinoDEV.h>
#include <ClearCore.h>
#include <math.h> // provides round(), pow()

// Pull in the global genie & encoder instances from your .ino
extern Genie                    genie;
extern ClearCore::EncoderInput  EncoderIn;

UIInputManager& UIInputManager::Instance() {
    static UIInputManager inst;
    return inst;
}

UIInputManager::UIInputManager()
    : countsPerClick(4)
{
    binding.active = false;
}

void UIInputManager::init(int counts) {
    countsPerClick = counts;
    binding.active = false;
}

void UIInputManager::bindField(uint8_t winButtonId,
    uint8_t ledDigitId,
    float* storagePtr,
    float min,
    float max,
    float step,
    uint8_t decimalPlaces)
{
    binding.winButtonId = winButtonId;
    binding.ledDigitId = ledDigitId;
    binding.valuePtr = storagePtr;
    binding.min = min;
    binding.max = max;
    binding.step = step;
    binding.decimalPlaces = decimalPlaces;
    // Disambiguate EncoderIn by fully qualifying
    binding.lastDetent = ClearCore::EncoderIn.Position() / countsPerClick;
    binding.active = true;

    // Write the initial value to the display
    float v = *binding.valuePtr;
    int32_t scaled = (int32_t)round(v * pow(10, decimalPlaces));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, binding.ledDigitId, (uint16_t)scaled);
}

void UIInputManager::unbindField() {
    binding.active = false;
}

void UIInputManager::update() {
    if (!binding.active) return;

    // Fully qualify EncoderIn to avoid ambiguity
    int32_t raw = ClearCore::EncoderIn.Position();
    int32_t detent = raw / countsPerClick;
    int32_t delta = detent - binding.lastDetent;
    if (delta == 0) return;

    // Compute new value and clamp
    float newVal = *binding.valuePtr + binding.step * delta;
    if (newVal > binding.max) newVal = binding.max;
    if (newVal < binding.min) newVal = binding.min;
    *binding.valuePtr = newVal;

    // Update the display
    int32_t scaled = (int32_t)round(newVal * pow(10, binding.decimalPlaces));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, binding.ledDigitId, (uint16_t)scaled);

    binding.lastDetent = detent;
}
