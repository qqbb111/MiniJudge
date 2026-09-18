#include "Cgroup.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <fcntl.h>

#include <unistd.h> // getpid, usleep

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
    if (!writeControlFile(cgroupPath / "pids.max", 64)) {
        cleanupCgroup(cgroupPath);
        return false;
    }

    return true;
}

bool joinCgroup(const std::string &procsPath) {
    int fd = open(procsPath.c_str(), O_WRONLY);
    if (fd == -1) return false;
    if (write(fd, "0\n", 2) != 2) {
        close(fd);
        return false;
    }
    close(fd);
    return true;
}

bool killCgroup(const std::string &path) {
    fs::path cgroupPath = path;
    fs::path killPath = cgroupPath / "cgroup.kill";
    fs::path eventsPath = cgroupPath / "cgroup.events";

    std::ofstream killFile(killPath);
    if (!killFile) {
        std::cerr << "Failed to open cgroup.kill: " << killPath << '\n';
        return false;
    }

    killFile << 1;
    killFile.close();

    if (killFile.fail()) {
        std::cerr << "Failed to kill cgroup: " << path << '\n';
        return false;
    }

    while (true) { // 轮询等待 cgroup 中所有进程退出（populated 变为 0）
        std::ifstream eventsFile(eventsPath);
        if (!eventsFile) {
            std::cerr << "Failed to open cgroup.events: " << eventsPath << '\n';
            return false;
        }

        std::string key;
        long long value;
        bool found = false;

        while (eventsFile >> key >> value) {
            if (key == "populated") {
                found = true;
                if (value == 0) {
                    return true;
                }
                break;
            }
        }

        if (!found) {
            std::cerr << "Failed to find populated in cgroup.events\n";
            return false;
        }

        usleep(1000);
    }
}

bool readOomKillCount(const std::string &cgroupPath, long long &count) {
    fs::path eventsPath = fs::path(cgroupPath) / "memory.events";
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

bool readMemoryPeak(const std::string &cgroupPath, long long &peakBytes) {
    fs::path peakPath = fs::path(cgroupPath) / "memory.peak";
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

bool readCpuUsage(const std::string &cgroupPath, long long &usageUsec) {
    fs::path cpuStatPath = fs::path(cgroupPath) / "cpu.stat";
    std::ifstream file(cpuStatPath);

    if (!file) {
        std::cerr << "Failed to open cpu.stat: " << cpuStatPath << '\n';
        return false;
    }

    std::string key;
    long long value;

    while (file >> key >> value) {
        if (key == "usage_usec") {
            usageUsec = value;
            return true;
        }
    }

    std::cerr << "Failed to find usage_usec in cpu.stat: " << cpuStatPath << '\n';
    return false;
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
