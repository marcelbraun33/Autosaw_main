#pragma once
#include "Screen.h"
#include "cutData.h"
class ScreenManager;

class JogZScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;
    JogZScreen(ScreenManager& mgr);
private:
    void setRPM(float value);
    void toggleEnable();
    ScreenManager& _mgr;
};
