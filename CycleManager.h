#pragma once
#include "Cycle.h"

class CycleManager {
public:
    static CycleManager& Instance();

    void startSemiAutoCycle(CutData& cutData);
    void startAutoCutCycle(CutData& cutData);


    void pauseCycle();
    void resumeCycle();
    void cancelCycle();
    void update();

    bool hasActiveCycle() const;
    Cycle* currentCycle();

private:
    Cycle* _currentCycle = nullptr;
    CycleManager() = default;
};
