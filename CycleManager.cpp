// controllers/CycleManager.cpp
#include "CycleManager.h"

CycleManager::CycleManager(std::vector<std::unique_ptr<ICycle>>&& c)
    : cycles(std::move(c)), current(0), running(false) {
}

void CycleManager::start() {
    if (cycles.empty()) return;
    running = true;
    current = 0;
    cycles[0]->start();
}

void CycleManager::stopCurrent() {
    if (running && current < cycles.size())
        cycles[current]->stop();
    running = false;
}

bool CycleManager::update() {
    if (!running) return false;

    auto& cycle = cycles[current];
    if (cycle->hasError()) {
        running = false;
        return false;
    }

    if (cycle->isDone()) {
        current++;
        if (current >= cycles.size()) {
            running = false;
            return false;
        }
        else {
            cycles[current]->start();
        }
    }
    return running;
}

bool CycleManager::hasError() const {
    if (current < cycles.size())
        return cycles[current]->hasError();
    return false;
}

CycleError CycleManager::error() const {
    if (current < cycles.size())
        return cycles[current]->error();
    return CycleError::None;
}

size_t CycleManager::currentIndex() const {
    return current;
}
