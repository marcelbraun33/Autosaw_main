
#pragma once
#include "Screen.h"
#include "Config.h"


class JogXScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    void update() override;

private:
    void captureZero();
    void captureStockLength();
    void captureIncrement();
    void calculateTotalSlices();
    void calculateAndSetIncrement();
    void goToZero();  // New method for "Go To Zero" button

    /// Set the global increment, enforce limits, recalc dependents & refresh all displays
    void setIncrement(float newIncrement);

    // Helper methods for updating LED displays
    void updatePositionDisplay();
    void updateStockLengthDisplay();
    void updateIncrementDisplay();
    void updateThicknessDisplay();
    void updateTotalSlicesDisplay();
    void updateSliceCounterDisplay();
};

