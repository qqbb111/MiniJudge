#pragma once
#include <string>

enum class RunStatus { Accepted, WrongAnswer, RuntimeError, TimeLimitExceeded, MemoryLimitExceeded, InternalError };

struct RunResult {
    RunStatus status;
    std::string detail;
    long long timeUs;
    long long memoryBytes;
};

RunResult run(const std::string &exePath, const std::string &inputPath, const std::string &actualOutputPath, const std::string &expectedPath, const std::string &cgroupPath, long long timeLimitMs,
              long long memoryLimitMiB);

bool isCoreDumping(pid_t pid);
