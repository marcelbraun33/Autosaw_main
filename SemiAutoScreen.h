#pragma once
#include "Screen.h"
#include "cutData.h"
class ScreenManager;

class SemiAutoScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    SemiAutoScreen(ScreenManager& mgr);
private:
    void startFeedToStop();
    void advanceIncrement();
    void feedHold();
    void exitFeedHold();
    ScreenManager& _mgr;
};

