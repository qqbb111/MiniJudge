#include <iostream>
#include <string>
#include <vector>

#include <filesystem>
#include <iomanip>  // setprecision
#include <getopt.h> // getopt_long
#include <charconv>
#include <unistd.h> // getpid

#include <thread>

#include "Compiler.h"
#include "Runner.h"
#include "Checker.h"
#include "TestCasesFinder.h"

namespace fs = std::filesystem;

struct TestResult {
    std::string name;
    std::string verdict;
    long long timeUs;
    long long memoryBytes;
};

TestResult judgeOneTest(const std::string &name, const fs::path &testDir, const fs::path &workDir, const fs::path &exePath, long long timeLimitMs, long long memoryLimitMiB) {
    fs::path input = testDir / (name + ".in");
    fs::path expected = testDir / (name + ".out");
    fs::path actualOutput = workDir / ("actual_" + name + ".out");
    fs::path cgroupPath = fs::path("/sys/fs/cgroup/minijudge") / ("run-" + std::to_string(getpid()) + "-" + name);

    RunResult runResult = run(exePath.string(), input.string(), actualOutput.string(), cgroupPath.string(), timeLimitMs, memoryLimitMiB);
    switch (runResult.status) {
        case RunStatus::RuntimeError:
            return {name, "RE", runResult.timeUs, runResult.memoryBytes};

        case RunStatus::TimeLimitExceeded:
            return {name, "TLE", runResult.timeUs, runResult.memoryBytes};

        case RunStatus::MemoryLimitExceeded:
            return {name, "MLE", runResult.timeUs, runResult.memoryBytes};

        case RunStatus::InternalError:
            return {name, "Run failed", runResult.timeUs, runResult.memoryBytes};

        case RunStatus::Ok:
            break;
    }

    CompareResult compareResult = compare(actualOutput.string(), expected.string());
    switch (compareResult) {
        case CompareResult::Accepted:
            return {name, "AC", runResult.timeUs, runResult.memoryBytes};

        case CompareResult::WrongAnswer:
            return {name, "WA", runResult.timeUs, runResult.memoryBytes};

        case CompareResult::Error:
            return {name, "Judge failed", runResult.timeUs, runResult.memoryBytes};
    }

    return {name, "Judge failed", runResult.timeUs, runResult.memoryBytes};
}

const char *shortOptions = "t:m:h";
static option longOptions[] = {{"time-limit", required_argument, nullptr, 't'}, {"memory-limit", required_argument, nullptr, 'm'}, {"help", no_argument, nullptr, 'h'}, {nullptr, 0, nullptr, 0}};

int main(int argc, char *argv[]) {
    int opt;
    long long timeLimitMs = 1000;
    long long memoryLimitMiB = 64;
    while ((opt = getopt_long(argc, argv, shortOptions, longOptions, nullptr)) != -1) {
        switch (opt) {
            case 't': {
                std::string timeText = optarg;
                auto result = std::from_chars(timeText.data(), timeText.data() + timeText.size(), timeLimitMs);
                if (result.ec != std::errc{} || result.ptr != timeText.data() + timeText.size()) {
                    std::cerr << "Invalid time limit\n";
                    return 1;
                }
                if (timeLimitMs <= 0) {
                    std::cerr << "Time limit must be positive\n";
                    return 1;
                }
                break;
            }
            case 'm': {
                std::string memoryText = optarg;
                auto result = std::from_chars(memoryText.data(), memoryText.data() + memoryText.size(), memoryLimitMiB);
                if (result.ec != std::errc{} || result.ptr != memoryText.data() + memoryText.size()) {
                    std::cerr << "Invalid memory limit\n";
                    return 1;
                }
                if (memoryLimitMiB <= 0) {
                    std::cerr << "Memory limit must be positive\n";
                    return 1;
                }
                break;
            }
            case 'h':
                std::cout << "Usage: " << argv[0] << " [options] <source_path>\n\nOptions:\n"
                          << "  -t, --time-limit <ms>     Set time limit in milliseconds (default: 1000 ms)\n"
                          << "  -m, --memory-limit <MiB>  Set memory limit in MiB (default: 64 MiB)\n"
                          << "  -h, --help                Show this help message\n";
                return 0;
            default:
                return 1;
        }
    }
    if (argc - optind != 1) {
        std::cerr << "Usage: " << argv[0] << " [options] <source_path>\n";
        return 1;
    }

    std::string codePath = argv[optind];
    std::string testDir = "tests";
    fs::path workDir = fs::path("tmp") / ("run-" + std::to_string(getpid()));
    fs::path exePath = workDir / "user_program";
    fs::path compileLog = workDir / "compile.log";

    std::error_code ec;
    bool created = fs::create_directories(workDir, ec);
    if (ec) {
        std::cerr << "Failed to create workDir " << workDir << ": " << ec.message() << '\n';
        return 1;
    }
    if (!created) {
        std::cerr << "Failed to create workDir: path already exists: " << workDir << '\n';
        return 1;
    }

    std::string errMessage;
    std::vector<std::string> testNames;
    if (!findTestCases(testDir, testNames, errMessage)) {
        std::cerr << errMessage << '\n';
        return 1;
    }

    if (!compile(codePath, exePath.string(), compileLog.string())) {
        std::cout << codePath << " CE\n";
        return 0;
    }

    std::vector<TestResult> results(testNames.size());
    std::vector<std::thread> threads;

    for (std::size_t i = 0; i < testNames.size(); i++) {
        threads.emplace_back([&, i]() { // 默认其他变量按引用捕获；i 按值捕获，每个线程保存自己的测试点下标
            results[i] = judgeOneTest(testNames[i], testDir, workDir, exePath, timeLimitMs, memoryLimitMiB);
        });
    }
    for (std::thread &thread : threads) thread.join();
    for (const TestResult &result : results)
        std::cout << std::fixed << std::setprecision(3) << "Test " << result.name << ": " << result.verdict << " (" << result.timeUs / 1000.0 << " ms, " << result.memoryBytes / 1024.0 / 1024.0 << " MiB)\n";

    std::error_code cleanupEc;
    fs::remove_all(workDir, cleanupEc);
    if (cleanupEc) {
        std::cerr << "Failed to remove workDir " << workDir << ": " << cleanupEc.message() << '\n'; // 评测已经成功完成，只是临时目录删除失败，不应该把 AC/WA/TLE 等评测结果推翻
    }

    return 0;
}
