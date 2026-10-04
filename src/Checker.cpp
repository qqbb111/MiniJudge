#include "Checker.h"

#include <algorithm> // min
#include <fstream>
#include <string>
#include <vector>

bool readOutput(const std::string &path, std::vector<std::string> &lines) {
    std::ifstream file(path);
    if (!file) return false;
    std::string line;

    while (std::getline(file, line)) {
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
            line.pop_back();
        }
        if (line.empty()) continue; // 忽略空行以及只包含行末空白的行
        lines.push_back(line);
    }

    if (file.bad()) return false; // 正常读到 EOF 不算错误；bad() 才表示真正的读取错误

    return true;
}

CompareResult compare(const std::string &actualPath, const std::string &expectedPath) {
    std::vector<std::string> actualLines;
    std::vector<std::string> expectedLines;

    if (!readOutput(actualPath, actualLines)) return {CompareStatus::Error};
    if (!readOutput(expectedPath, expectedLines)) return {CompareStatus::Error};
    if (actualLines == expectedLines) return {CompareStatus::Accepted};
    std::size_t n = std::min(actualLines.size(), expectedLines.size());
    std::size_t line = n;
    for (std::size_t i = 0; i < n; i++) {
        if (actualLines[i] != expectedLines[i]) {
            line = i;
            break;
        }
    }
    if (line == n) {
        if (actualLines.size() < expectedLines.size())
            return {CompareStatus::WrongAnswer, WaDetail{line + 1, expectedLines[line], "<EOF>"}};
        else
            return {CompareStatus::WrongAnswer, WaDetail{line + 1, "<EOF>", actualLines[line]}};
    } else
        return {CompareStatus::WrongAnswer, WaDetail{line + 1, expectedLines[line], actualLines[line]}};
}
