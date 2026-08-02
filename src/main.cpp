#include <iostream>
#include <string>

#include "Compiler.h"
#include "Runner.h"
#include "Checker.h"

int main(){
    std::string code = "examples/accepted.cpp";
    std::string exe = "tmp/user_program";
    int testcnt = 2;

    if(!compile(code, exe)){
        std::cout << code << " CE\n";
        return 0;
    }

    for(int i = 1; i <= testcnt; i++){
        std::string input = "tests/" + std::to_string(i) + ".in";
        std::string output = "tmp/actual" + std::to_string(i) + ".out";
        if(!run(exe, input, output)){
            std::cout << "Run failed on test " << i << '\n';
            continue;
        }

        std::string expected = "tests/" + std::to_string(i) + ".out";
        std::cout << "Test" << i << ": Run OK ";
        if(compare(output, expected)) std::cout << "AC\n";
        else std::cout << "WA\n";
    }

    return 0;
}
