#pragma once
#include <string>

enum class CompareResult { Accepted, WrongAnswer, Error };

CompareResult compare(const std::string &actualPath, const std::string &expectedPath);
