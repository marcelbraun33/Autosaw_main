#pragma once
#include "Screen.h"
#include "Config.h"
#include "CutData.h"

// Forward declaration instead of including ScreenManager.h
class ScreenManager;

class JogXScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    void update() override;
    JogXScreen(ScreenManager& mgr);

    // Add a static getter for cut thickness (adjust as needed)
    static float GetCutThickness();

private:
    // UI and logic helpers
    void captureZero();
    void captureStockLength();
    void captureIncrement();
    void calculateTotalSlices();
    void setStockSlicesTimesIncrement(); // Button 51 handler
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

protected:
    static float m_cutThickness; // Ensure this holds the current cut thickness

    ScreenManager& _mgr;
};
