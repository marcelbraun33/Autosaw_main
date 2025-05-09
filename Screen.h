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

protected:
    // Safe WinButton state writer with delay to avoid redraw conflict
    void showButtonSafe(uint16_t winButtonId, uint16_t value = 1, uint16_t delayMs = 30) {
        delay(delayMs);
        extern Genie genie;
        genie.WriteObject(GENIE_OBJ_WINBUTTON, winButtonId, value);
    }
};
