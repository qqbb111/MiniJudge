#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <iomanip> // setprecision
#include <getopt.h> // getopt_long
#include <charconv>

#include "Compiler.h"
#include "Runner.h"
#include "Checker.h"
#include "TestCasesFinder.h"

const char* shortOptions = "t:h";
static option longOptions[] = {
    {"time-limit", required_argument, nullptr, 't'},
    {"help", no_argument, nullptr, 'h'},
    {nullptr, 0, nullptr, 0}
};

int main(int argc, char* argv[]){
    int opt;
    long long timeLimitMs = 1000;
    while((opt = getopt_long(argc, argv, shortOptions, longOptions, nullptr)) != -1){
        switch (opt){
            case 't':{
                std::string timeText = optarg;
                auto result = std::from_chars(
                    timeText.data(),
                    timeText.data() + timeText.size(),
                    timeLimitMs
                );
                if(result.ec != std::errc{} || 
                   result.ptr != timeText.data() + timeText.size()){
                    std::cerr << "Invalid time limit\n";
                    return 1;
                }
                if(timeLimitMs <= 0){
                    std::cerr << "Time limit must be positive\n";
                    return 1;
                }
                break;
            }
            case 'h':
                std::cout << "Usage: "<< argv[0] <<" [options] <source_path>\n\nOptions:\n"
                          << "  -t, --time-limit <ms>  Set time limit in milliseconds (default: 1000)\n"
                          << "  -h, --help             Show this help message\n";
                return 0;
            default:
                return 1;
        }
    }
    if(argc - optind != 1){
        std::cerr << "Usage: " << argv[0] << " [options] <source_path>\n";
        return 1;
    }

    std::string testDir = "tests";
    std::string errMessage;
    std::vector<std::string> testNames;
    if(!findTestCases(testDir, testNames, errMessage)){
        std::cerr << errMessage << '\n';
        return 1;
    }

    std::string code = argv[optind];
    std::string exe = "tmp/user_program";
    if(!compile(code, exe)){
        std::cout << code << " CE\n";
        return 0;
    }


    std::cout << std::fixed << std::setprecision(3);
    for(const std::string& name : testNames){
        std::filesystem::path input = std::filesystem::path(testDir) / (name + ".in");
        std::filesystem::path expected = std::filesystem::path(testDir) / (name + ".out");
        std::filesystem::path actualOutput = std::filesystem::path("tmp") / ("actual_" + name + ".out");

        RunResult runResult = run(exe, input.string(), actualOutput.string(), timeLimitMs);
        std::cout << "Test " << name << ": ";
        if(runResult.status == RunStatus::RuntimeError){
            std::cout << "RE (" << runResult.timeUs / 1000.0 << " ms)\n";
            continue;
        }
        if(runResult.status == RunStatus::InternalError){
            std::cout << "Run failed (" << runResult.timeUs / 1000.0 << " ms)\n";
            continue;
        }
        if(runResult.status == RunStatus::TimeLimitExceeded){
            std::cout << "TLE (" << runResult.timeUs / 1000.0 << " ms)\n";
            continue;
        }

        if(compare(actualOutput.string(), expected.string())){ 
            std::cout << "AC (" << runResult.timeUs / 1000.0 << " ms)\n";
        }
        else{
            std::cout << "WA (" << runResult.timeUs / 1000.0 << " ms)\n";
        }
    }

    return 0;
}
