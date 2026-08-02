#include "Compiler.h"
#include <string>
#include <cstdlib>

bool compile(const std::string& code, const std::string& exe){ // 编译
    std::string cmd = "g++ " + code + " -std=c++17 -O2 -o " + exe + " 2> tmp/compile.log";
    int res = std::system(cmd.c_str()); // C标准库函数system()需要的参数类型是 const char* 传统的C风格字符串
    return !res;
}