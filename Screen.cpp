// Updated Screen.cpp
#include "Screen.h"
#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include <stdint.h>
#include "Config.h"

extern Genie genie;  // reference to global UI instance

// Screen destructor
Screen::~Screen() {
    // Virtual destructor implementation
}

// Default implementation for onHide
void Screen::onHide() {
    // Default does nothing
}

// Default implementation for update
void Screen::update() {
    // Default does nothing
}

// Clear all LED digits to prevent floating digits
void Screen::clearAllLedDigits() {
    // Debug output
    ClearCore::ConnectorUsb.SendLine("[Screen] Clearing all LED digits");

    // Clear all LED digits (0-35 covers all used digits)
    for (uint8_t i = 0; i < 35; i++) {
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, i, 0);
    }
}

// Safe button state writer with delay
void Screen::showButtonSafe(uint16_t winButtonId, uint16_t value, uint16_t delayMs) {
    // Implement the button safety mechanism
    genie.WriteObject(GENIE_OBJ_WINBUTTON, winButtonId, value);
    if (delayMs > 0) {
        delay(delayMs);
    }
}

// Ensure these helpers exist for LED/LED digit control

void ScreenManager::SetLEDDigits(int startDigit, float value) {
    // Update the LED digits to display the value starting at startDigit
    // Implementation depends on your hardware/display library
}

void ScreenManager::SetLED(int ledIndex, bool on) {
    // Set the specified LED (e.g., LED 4) ON or OFF
    // Implementation depends on your hardware/display library
}
