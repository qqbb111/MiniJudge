#include "Checker.h"
#include "Runner.h"

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

RunResult compare(const std::string &actualPath, const std::string &expectedPath) {
    std::vector<std::string> actualLines;
    std::vector<std::string> expectedLines;

    if (!readOutput(actualPath, actualLines)) return {RunStatus::InternalError, "", 0, 0};
    if (!readOutput(expectedPath, expectedLines)) return {RunStatus::InternalError, "", 0, 0};

    size_t n = std::min(actualLines.size(), expectedLines.size());

    for (size_t i = 0; i < n; ++i) {
        if (actualLines[i] != expectedLines[i]) {
            return {RunStatus::WrongAnswer, "line " + std::to_string(i + 1) + "\nexpected: " + expectedLines[i] + "\nactual  : " + actualLines[i], 0, 0};
        }
    }

    if (actualLines.size() != expectedLines.size()) {
        return {RunStatus::WrongAnswer, "line count differs", 0, 0};
    }

    return {RunStatus::Accepted, "", 0, 0};
}
