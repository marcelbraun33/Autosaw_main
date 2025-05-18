#pragma once
#include "Screen.h"

class JogYScreen : public Screen {
public:
    void onShow() override;
    void onHide() override;
    void handleEvent(const genieFrame& e) override;

private:
    void captureCutStart();
    void captureCutEnd();
    void updateCutLengthDisplay();
};
