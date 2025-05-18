#pragma once
#include "Screen.h"

class JogZScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;

private:
    void setRPM(float value);
    void toggleEnable();
};
