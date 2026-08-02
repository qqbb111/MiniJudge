#include"Checker.h"
#include<string>
#include<cstdlib>

bool compare(const std::string& actual, const std::string& expected){
    std::string cmd = "diff -wB " + actual + " " + expected + " > /dev/null 2>&1";
    int res = std::system(cmd.c_str());
    return !res;
}