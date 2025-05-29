#pragma once
#include "Screen.h"
#include "Config.h"
#include "CutData.h"
class ScreenManager;

class JogXScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    void update() override;
    JogXScreen(ScreenManager& mgr);

private:
    // UI and logic helpers
    void captureZero();
    void captureStockLength();
    void captureIncrement();
    void calculateTotalSlices();
    void calculateAndSetIncrement();
    void goToZero();  // "Go To Zero" button

    /// Set the global increment, enforce limits, recalc dependents & refresh all displays
    void setIncrement(float newIncrement);

    // Helper methods for updating LED displays
    void updatePositionDisplay();
    void updateStockLengthDisplay();
    void updateIncrementDisplay();
    void updateThicknessDisplay();
    void updateTotalSlicesDisplay();
    void updateSliceCounterDisplay();

    ScreenManager& _mgr;
};
