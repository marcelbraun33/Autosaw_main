#pragma once
#include "Screen.h"

class AutoCutScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;

private:
    void pauseCycle();
    void resumeCycle();
    void cancelCycle();
};
