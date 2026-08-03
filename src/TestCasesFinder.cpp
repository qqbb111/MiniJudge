#include "TestCasesFinder.h"

#include <filesystem>
#include <set>

bool findTestCases(const std::string& testDir, std::vector<std::string>& testNames, std::string& errMessage){
    testNames.clear();
    errMessage.clear();
    if(!std::filesystem::exists(testDir)){
        errMessage = "No testDir found";
        return false;
    }
    if(!std::filesystem::is_directory(testDir)){
        errMessage = "TestDir is not a directory: " + testDir;
        return false;
    }

    std::set<std::string> inputNames, outputNames;
    for(const auto& it : std::filesystem::directory_iterator(testDir)){
        if(!it.is_regular_file()) continue;
        std::filesystem::path path = it.path();
        std::string filename = path.filename().string();
        
        if(filename == ".in" || filename == ".out"){
            errMessage = "Invalid empty test case name: " + filename;
            return false;
        }

        std::string suf = path.extension().string();
        std::string pre = path.stem().string();
        if(suf == ".in" || suf == ".out"){
            for(char ch : pre){
                if(!(('a' <= ch && ch <= 'z') ||
                     ('A' <= ch && ch <= 'Z') ||
                     ('0' <= ch && ch <= '9') ||
                     ch == '_' || ch == '-' ||
                     ch == '#' || ch == '.')){
                    errMessage = "Invalid test case name: " + pre;
                    return false;
                }
            }
            if(suf == ".in") inputNames.insert(pre);
            else outputNames.insert(pre);
        }
    }

    if(inputNames.empty() && outputNames.empty()){
        errMessage = "No test cases found";
        return false;
    }

    for(const std::string& it : inputNames){
        if(outputNames.count(it) == 0){
            errMessage = "Missing output test case for test " + it;
            return false;
        }
    }
    for(const std::string& it : outputNames){
        if(inputNames.count(it) == 0){
            errMessage = "Missing input test case for test " + it;
            return false;
        }
    }

    testNames.assign(inputNames.begin(), inputNames.end());

    return true;
}
