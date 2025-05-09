#pragma once

class AutoSawController {
public:
    static AutoSawController& Instance();

    void setup();
    void update();

private:
    AutoSawController() = default;
    ~AutoSawController() = default;

    AutoSawController(const AutoSawController&) = delete;
    AutoSawController& operator=(const AutoSawController&) = delete;
    AutoSawController(AutoSawController&&) = delete;
    AutoSawController& operator=(AutoSawController&&) = delete;
};
