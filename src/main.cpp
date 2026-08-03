#include <iostream>
#include <string>
#include <filesystem>
#include <vector>

#include "Compiler.h"
#include "Runner.h"
#include "Checker.h"
#include "TestCasesFinder.h"


int main(int argc, char* argv[]){
    if(argc < 2){
        std::cerr << "Usage: " << argv[0] << " <source_path>\n";
        return 1;
    }

    
    std::string testDir = "tests";
    std::string errMessage;
    std::vector<std::string> testNames;
    if(!findTestCases(testDir, testNames, errMessage)){
        std::cerr << errMessage << '\n';
        return 1;
    }

    std::string code = argv[1];
    std::string exe = "tmp/user_program";
    if(!compile(code, exe)){
        std::cout << code << " CE\n";
        return 0;
    }

    for(const std::string& name : testNames){
        std::filesystem::path input = std::filesystem::path(testDir) / (name + ".in");
        std::filesystem::path expected = std::filesystem::path(testDir) / (name + ".out");
        std::filesystem::path actualOutput = std::filesystem::path("tmp") / ("actual_" + name + ".out");
        if(!run(exe, input.string(), actualOutput.string())){
            std::cout << "Run failed on test " << name << '\n';
            continue;
        }

        std::cout << "Test " << name << ": ";
        if(compare(actualOutput.string(), expected.string())){ 
            std::cout << "AC\n";
        }
        else{
            std::cout << "WA\n";
        }
    }

    return 0;
}
