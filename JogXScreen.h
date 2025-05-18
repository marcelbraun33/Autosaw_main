#pragma once
#include "Screen.h"

class JogXScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;

private:
    void captureZero();
    void captureStockLength();
    void captureIncrement();
    void calculateTotalSlices();
};
