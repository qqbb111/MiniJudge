#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

int main() {
    const int testCount = 50;

    std::mt19937 rng(20050620);
    std::uniform_int_distribution<int> dist(-1000000, 1000000);

    for (int i = 1; i <= testCount; ++i) {
        int a = dist(rng);
        int b = dist(rng);

        std::ostringstream name;
        name << "random_" << std::setw(3) << std::setfill('0') << i;

        std::ofstream input("tests/" + name.str() + ".in");
        std::ofstream output("tests/" + name.str() + ".out");

        input << a << ' ' << b << '\n';
        output << a + b << '\n';
    }

    return 0;
}
