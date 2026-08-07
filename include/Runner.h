#pragma once
#include <string>

struct RunResult{
    int status; // 0 -> OK, 1 -> RE, 2 -> Run failed
    long long elapsedMicroseconds;
};

RunResult run(const std::string& exePath, const std::string& inputPath, const std::string& actualOutputPath);
