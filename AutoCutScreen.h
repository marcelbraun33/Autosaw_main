#pragma once
#include "Screen.h"
#include "cutData.h"
class ScreenManager;

class AutoCutScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    AutoCutScreen(ScreenManager& mgr);
private:
    void pauseCycle();
    void resumeCycle();
    void cancelCycle();
    ScreenManager& _mgr;
};
