#include <fstream>
#include <iostream>
#include <string>

int main() {
    std::ifstream f("/tmp/host_secret");
    std::string s;
    std::getline(f, s);
    std::cout << (f ? s : "ACCESS DENIED") << '\n';
}
