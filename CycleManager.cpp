#include "CycleManager.h"
#include "SemiAutoCycle.h"
#include "AutoCutCycle.h"

CycleManager& CycleManager::Instance() {
    static CycleManager instance;
    return instance;
}

void CycleManager::startSemiAutoCycle(CutData& cutData) {
    if (_currentCycle) {
        delete _currentCycle;
    }
    _currentCycle = new SemiAutoCycle(cutData);
    _currentCycle->start();
}

void CycleManager::startAutoCutCycle(CutData& cutData) {
    if (_currentCycle) {
        delete _currentCycle;
    }
    _currentCycle = new AutoCutCycle(cutData);
    _currentCycle->start();
}

void CycleManager::pauseCycle() {
    if (_currentCycle) _currentCycle->pause();
}

void CycleManager::resumeCycle() {
    if (_currentCycle) _currentCycle->resume();
}

void CycleManager::cancelCycle() {
    if (_currentCycle) _currentCycle->cancel();
}

void CycleManager::update() {
    if (_currentCycle) _currentCycle->update();
    if (_currentCycle && _currentCycle->isComplete()) {
        delete _currentCycle;
        _currentCycle = nullptr;
    }
}


bool CycleManager::hasActiveCycle() const {
    return _currentCycle && _currentCycle->isActive();
}

Cycle* CycleManager::currentCycle() {
    return _currentCycle;
}
