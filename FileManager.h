#pragma once

class FileManager {
public:
    static FileManager& Instance() {
        static FileManager instance;
        return instance;
    }

private:
    FileManager() = default;
};
