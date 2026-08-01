#include<bits/stdc++.h>
using namespace std;


bool compile(const string& code, const string& exe){ // 编译
    string cmd = "g++ " + code + " -std=c++17 -O2 -o " + exe + " 2> tmp/compile.log";
    int res = system(cmd.c_str()); // C标准库函数system()需要的参数类型是 const char* 传统的C风格字符串
    return !res;
}

bool run(const string& exe, const string& input, const string& output){ // 运行
    string cmd = "./" + exe + " < " + input + " > " + output;
    int res = system(cmd.c_str());
    return !res;
}

bool compare(const string& actual, const string& expected){
    string cmd = "diff -wB " + actual + " " + expected + " > /dev/null 2>&1";
    int res = system(cmd.c_str());
    return !res;
}

int main(){
    string code = "examples/accepted.cpp", exe = "tmp/user_program";
    int testcnt = 2;

    if(!compile(code, exe)){
        cout << code << " CE\n";
        return 0;
    }

    for(int i = 1; i <= testcnt; i++){
        string input = "tests/" + to_string(i) + ".in";
        string output = "tmp/actual" + to_string(i) + ".out";
        if(!run(exe, input, output)){
            cout << "Run failed on test " << i << '\n';
            continue;
        }

        string expected = "tests/" + to_string(i) + ".out";
        cout << "Test" << i << ": Run OK ";
        if(compare(output, expected)) cout << "AC\n";
        else cout << "WA\n";
    }

    return 0;
}