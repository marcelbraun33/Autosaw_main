// === HomingScreen.h ===
#pragma once

#include "Screen.h"
#include <ClearCore.h>

/// UI screen that waits for both X- and Y-axis HLFB and then returns to Manual Mode
class HomingScreen : public Screen {
public:
    void onShow() override;
    void onHide() override {}
    void handleEvent(const genieFrame&) override {}
    void update() override;

private:
    bool _xDone = false;
    bool _yDone = false;
};
