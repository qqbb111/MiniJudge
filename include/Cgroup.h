#pragma once
#include <string>

bool createCgroup(const std::string &path, long long memoryLimitBytes);

bool joinCgroup(const std::string &procsPath);

bool killCgroup(const std::string &path);

bool readOomKillCount(const std::string &cgroupPath, long long &count);
bool readMemoryPeak(const std::string &cgroupPath, long long &peakBytes);
bool readCpuUsage(const std::string &cgroupPath, long long &usageUsec);

bool removeCgroup(const std::string &path);
