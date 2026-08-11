#pragma once
#include <string>

enum class RunStatus{
    Ok,
    RuntimeError,
    TimeLimitExceeded,
    InternalError
};

struct RunResult{
    RunStatus status;
    long long timeUs;
};

RunResult run(const std::string& exePath, const std::string& inputPath, const std::string& actualOutputPath);

bool isCoreDumping(pid_t pid);
