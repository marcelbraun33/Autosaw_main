// UIInputManager.cpp
#include "UIInputManager.h"
#include "Config.h"          // for ENCODER_COUNTS_PER_CLICK
#include "MPGJogManager.h"
#include <genieArduinoDEV.h>
#include <ClearCore.h>
#include <cmath>
#include "ScreenManager.h"  


extern Genie genie;

UIInputManager& UIInputManager::Instance() {
    static UIInputManager inst;
    return inst;
}

UIInputManager::UIInputManager()
    : countsPerClick(ENCODER_COUNTS_PER_CLICK),
    _lastRaw(0)
{
    binding.active = false;
}

void UIInputManager::init(int counts) {
    countsPerClick = counts;
    binding.active = false;
    // seed baseline
    _lastRaw = ClearCore::EncoderIn.Position();
}

void UIInputManager::bindField(uint8_t win, uint8_t led, float* ptr,
    float mn, float mx, float st, uint8_t dp) {
    binding.winButtonId = win;
    binding.ledDigitId = led;
    binding.valuePtr = ptr;
    binding.min = mn;
    binding.max = mx;
    binding.step = st;
    binding.decimalPlaces = dp;
    binding.lastDetent = ClearCore::EncoderIn.Position() / countsPerClick;
    binding.active = true;

    // write initial value
    float v = *binding.valuePtr;
    int32_t scaled = (int32_t)round(v * pow(10, dp));
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, binding.ledDigitId, (uint16_t)scaled);
}

void UIInputManager::unbindField() {
    binding.active = false;
}

bool UIInputManager::isFieldActive(uint16_t b) {
    return binding.active && binding.winButtonId == b;
}

bool UIInputManager::isEditing() const {
    return binding.active;
}

void UIInputManager::resetRaw() {
    _lastRaw = ClearCore::EncoderIn.Position();
}

void UIInputManager::update() {
    // field‐editing mode?
    if (binding.active) {
        int32_t raw = ClearCore::EncoderIn.Position();
        int32_t detent = raw / countsPerClick;
        int32_t delta = detent - binding.lastDetent;
        if (delta != 0) {
            float newVal = *binding.valuePtr + binding.step * delta;
            if (newVal < binding.min) newVal = binding.min;
            else if (newVal > binding.max) newVal = binding.max;
            *binding.valuePtr = newVal;

            int32_t scaled = (int32_t)round(newVal * pow(10, binding.decimalPlaces));
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, binding.ledDigitId, (uint16_t)scaled);

            binding.lastDetent = detent;
        }
        return;
    }

    // jog mode?
    if (MPGJogManager::Instance().isEnabled()
        && ScreenManager::Instance().currentForm() == FORM_JOG_X) {

        int32_t rawDelta = ClearCore::EncoderIn.Position() - _lastRaw;
        int32_t clicks = rawDelta / countsPerClick;
        if (clicks != 0) {
            int range = JOG_MULTIPLIER_X1;
            if (RANGE_PIN_X10.State())  range = JOG_MULTIPLIER_X10;
            if (RANGE_PIN_X100.State()) range = JOG_MULTIPLIER_X100;

            MPGJogManager::Instance().onEncoderDelta(clicks * range);
            _lastRaw = ClearCore::EncoderIn.Position();
        }
    }
}
