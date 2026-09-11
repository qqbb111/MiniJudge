#include "Checker.h"

#include <fstream>
#include <string>
#include <vector>

void trimTrailingWhitespace(std::string &line) {
    while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
        line.pop_back();
    }
}

bool readOutput(const std::string &path, std::vector<std::string> &lines) {
    std::ifstream file(path);
    if (!file) return false;
    std::string line;

    while (std::getline(file, line)) {
        trimTrailingWhitespace(line);
        if (line.empty()) { // 忽略空行以及只包含行末空白的行
            continue;
        }
        lines.push_back(line);
    }

    if (file.bad()) return false; // 正常读到 EOF 不算错误；bad() 才表示真正的读取错误

    return true;
}

CompareResult compare(const std::string &actualPath, const std::string &expectedPath) {
    std::vector<std::string> actualLines;
    std::vector<std::string> expectedLines;

    if (!readOutput(actualPath, actualLines)) return CompareResult::Error;
    if (!readOutput(expectedPath, expectedLines)) return CompareResult::Error;
    if (actualLines == expectedLines) return CompareResult::Accepted;
    return CompareResult::WrongAnswer;
}
