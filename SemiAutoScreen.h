#pragma once
#include "Screen.h"

class SemiAutoScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;

private:
    void startFeedToStop();
    void advanceIncrement();
    void feedHold();
    void exitFeedHold();
};
