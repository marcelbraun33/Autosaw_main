#pragma once

class EStopManager {
public:
    static EStopManager& Instance() {
        static EStopManager instance;
        return instance;
    }

    void setup() {}
    void update() {}

private:
    EStopManager() = default;
};
