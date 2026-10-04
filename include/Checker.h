#pragma once
#include <string>
#include <optional>

enum class CompareStatus { Accepted, WrongAnswer, Error };

struct WaDetail {
    std::size_t line;
    std::string expected;
    std::string actual;
};

struct CompareResult {
    CompareStatus status;

    std::optional<WaDetail> waDetail = std::nullopt;
};

CompareResult compare(const std::string &actualPath, const std::string &expectedPath);
