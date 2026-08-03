#pragma once
#include <string>

struct RunResult{
    bool success;
    long long elapsedMicroseconds;
};

RunResult run(const std::string& exePath, const std::string& inputPath, const std::string& actualOutputPath);
