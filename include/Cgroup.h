#pragma once
#include <string>

bool createCgroup(const std::string &path, long long memoryLimitBytes);

bool joinCgroup(const std::string &path);

bool killCgroup(const std::string &path);

bool readOomKillCount(const std::string &path, long long &count);

bool readMemoryPeak(const std::string &path, long long &peakBytes);

bool removeCgroup(const std::string &path);
