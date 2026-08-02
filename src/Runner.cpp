#include "Runner.h"
#include <string>
#include <cstdlib>

bool run(const std::string& exe, const std::string& input, const std::string& output){ // 运行
    std::string cmd = "./" + exe + " < " + input + " > " + output;
    int res = std::system(cmd.c_str());
    return !res;
}
