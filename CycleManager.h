// === CycleManager.h ===
#pragma once

#include "ArduinoCompat.h"
#include "ICycle.h"
#include <vector>
#include <memory>

/// Sequences through a list of ICycle instances
class CycleManager {
public:
    CycleManager(std::vector<std::unique_ptr<ICycle>>&& cycles);

    /// Begin the first cycle.
    void start();

    /// Abort the currently running cycle.
    void stopCurrent();

    /// Poll once per loop: 
    ///   - advances to the next cycle when done,
    ///   - returns true if still running,
    ///   - returns false if all cycles are complete or an error occurred.
    bool update();

    bool hasError() const;
    CycleError error() const;

    /// For UI polling: which cycle index are we on? (0-based)
    size_t currentIndex() const;

private:
    std::vector<std::unique_ptr<ICycle>> cycles;
    size_t current = 0;
    bool running = false;
};
