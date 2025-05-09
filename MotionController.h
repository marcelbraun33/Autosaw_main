#pragma once

class MotionController {
public:
    static MotionController& Instance() {
        static MotionController instance;
        return instance;
    }

    void setup() {}

private:
    MotionController() = default;
};
