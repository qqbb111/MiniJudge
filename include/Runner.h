#pragma once
#include <string>

enum class RunStatus{
    Ok,
    RuntimeError,
    TimeLimitExceeded,
    MemoryLimitExceeded,
    InternalError
};

struct RunResult{
    RunStatus status;
    long long timeUs;
    long long memoryBytes;
};

RunResult run(const std::string& exePath, const std::string& inputPath, const std::string& actualOutputPath, long long timeLimitMs, long long memoryLimitMiB);

bool isCoreDumping(pid_t pid);
