#pragma once

#include <genieArduinoDEV.h>
#include <Arduino.h>  // Needed for delay()

// Abstract base class for all UI screens
class Screen {
public:
    virtual ~Screen() {}

    // Called when the screen is displayed
    virtual void onShow() = 0;

    // Called when the screen is hidden (optional override)
    virtual void onHide() {}

    // Handle incoming events from the Gen4 display
    virtual void handleEvent(const genieFrame& e) = 0;

    // Optional per-frame update
    virtual void update() {}

protected:
    // Simplified button writer with optional delay
    void showButtonSafe(uint16_t winButtonId, uint16_t value = 1, uint16_t delayMs = 0) {
        extern Genie genie;
        genie.WriteObject(GENIE_OBJ_WINBUTTON, winButtonId, value);
        // Only delay if explicitly requested
        if (delayMs > 0) {
            delay(delayMs);
        }
    }
};
