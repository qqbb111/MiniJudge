#include"Compiler.h"
#include"Runner.h"
#include"Checker.h"
#include<bits/stdc++.h>
using namespace std;

int main(){
    string code = "examples/accepted.cpp";
    string exe = "tmp/user_program";
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