#include "Cgroup.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>

#include <unistd.h> // getpid

namespace fs = std::filesystem;

namespace {
bool writeControlFile(const fs::path &filePath, long long value) {
    std::ofstream file(filePath);
    if (!file) {
        std::cerr << "Failed to open cgroup control file: " << filePath << '\n';
        return false;
    }
    file << value;
    file.close(); // 将文件流缓冲区中剩余的数据刷新到操作系统；关闭文件流与文件的连接；释放相关资源
    if (file.fail()) {
        std::cerr << "Failed to write cgroup control file: " << filePath << '\n';
        return false;
    }
    return true;
}

void cleanupCgroup(const fs::path &cgroupPath) {
    std::error_code ec;
    fs::remove(cgroupPath, ec);
    if (ec) std::cerr << "Failed to clean up cgroup " << cgroupPath << ": " << ec.message() << '\n';
}
} // namespace

bool createCgroup(const std::string &path, long long memoryLimitBytes) {
    if (memoryLimitBytes <= 0) {
        std::cerr << "Invalid memory limit: " << memoryLimitBytes << '\n';
        return false;
    }

    fs::path cgroupPath = path;

    std::error_code ec;
    bool created = fs::create_directory(cgroupPath, ec);

    if (ec) {
        std::cerr << "Failed to create cgroup " << cgroupPath << ": " << ec.message() << '\n';
        return false;
    }

    if (!created) {
        std::cerr << "Failed to create cgroup: path already exists: " << cgroupPath << '\n';
        return false;
    }

    if (!writeControlFile(cgroupPath / "memory.max", memoryLimitBytes)) {
        cleanupCgroup(cgroupPath);
        return false;
    }

    if (!writeControlFile(cgroupPath / "memory.swap.max", 0)) {
        cleanupCgroup(cgroupPath);
        return false;
    }

    return true;
}

bool joinCgroup(const std::string &path) {
    fs::path procsPath = fs::path(path) / "cgroup.procs";

    std::ofstream file(procsPath);

    if (!file) {
        std::cerr << "Failed to open cgroup.procs: " << procsPath << '\n';
        return false;
    }
    file << getpid();
    file.close();
    if (file.fail()) {
        std::cerr << "Failed to join cgroup: " << path << '\n';
        return false;
    }

    return true;
}

bool readOomKillCount(const std::string &path, long long &count) {
    fs::path eventsPath = fs::path(path) / "memory.events";
    std::ifstream file(eventsPath);

    if (!file) {
        std::cerr << "Failed to open memory.events: " << eventsPath << '\n';
        return false;
    }

    std::string key;
    long long value;

    while (file >> key >> value) {
        if (key == "oom_kill") {
            count = value;
            return true;
        }
    }

    std::cerr << "Failed to find oom_kill in memory.events: " << eventsPath << '\n';
    return false;
}

bool readMemoryPeak(const std::string &path, long long &peakBytes) {
    fs::path peakPath = fs::path(path) / "memory.peak";
    std::ifstream file(peakPath);
    
    if (!file) {
        std::cerr << "Failed to open memory.peak: " << peakPath << '\n';
        return false;
    }

    file >> peakBytes;

    if (!file) {
        std::cerr << "Failed to read memory.peak: " << peakPath << '\n';
        return false;
    }

    return true;
}

bool removeCgroup(const std::string &path) {
    std::error_code ec;
    bool removed = fs::remove(fs::path(path), ec);

    if (ec) {
        std::cerr << "Failed to remove cgroup " << path << ": " << ec.message() << '\n';
        return false;
    }

    if (!removed) {
        std::cerr << "Failed to remove cgroup: path does not exist: " << path << '\n';
        return false;
    }

    return true;
}
