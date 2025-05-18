// === AutoSawController.h ===
#pragma once

/// Main application controller: initializes subsystems and manages UI flow
class AutoSawController {
public:
    static AutoSawController& Instance();

    /// Perform all hardware and UI initialization
    void setup();

    /// Call each loop iteration to process events and updates
    void update();

private:
    AutoSawController() = default;
    ~AutoSawController() = default;
    AutoSawController(const AutoSawController&) = delete;
    AutoSawController& operator=(const AutoSawController&) = delete;
    AutoSawController(AutoSawController&&) = delete;
    AutoSawController& operator=(AutoSawController&&) = delete;
};
