#include "Runner.h"
#include <string>
#include <cstdlib>
#include <chrono>

RunResult run(const std::string& exePath, const std::string& inputPath, const std::string& actualOutputPath){ // 运行
    std::string cmd = "./" + exePath + " < " + inputPath + " > " + actualOutputPath;
    auto start = std::chrono::steady_clock::now();
    int res = std::system(cmd.c_str());
    auto end = std::chrono::steady_clock::now();
    long long elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    return {res == 0, elapsedUs};
}
