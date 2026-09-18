#include "Runner.h"
#include "Cgroup.h"
#include "Checker.h"

#include <string>
#include <chrono>
#include <unistd.h>   // fork, dup2, close, execv, _exit, pipe
#include <sys/wait.h> // waitpid, waitpid, WIFEXITED, WEXITSTATUS
#include <fcntl.h>    // open, O_RDONLY...
#include <cstdio>     // perror
#include <signal.h>   // SIGKILL
#include <fstream>    // ifstream
#include <iostream>

bool isCoreDumping(pid_t pid) {
    std::string path = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream file(path);
    std::string line;

    while (std::getline(file, line)) {
        if (line.find("CoreDumping:") != std::string::npos) {
            size_t pos = line.find(':');
            int val = std::stoi(line.substr(pos + 1));
            return val == 1;
        }
    }
    return false;
}

RunResult run(const std::string &exePath, const std::string &inputPath, const std::string &actualOutputPath, const std::string &expectedPath, const std::string &cgroupPath, long long timeLimitMs,
              long long memoryLimitMiB) {
    auto start = std::chrono::steady_clock::now();
    auto getElapsedUs = [&start]() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count(); };

    long long peakMemoryBytes = 0ll;

    int pipeFd[2];                        // fork 之前创建管道，供父子进程间通信
    if (pipe2(pipeFd, O_CLOEXEC) == -1) { /* 如果不用 pipe2 的 O_CLOEXEC 参数（Close On Exec），保证 execv 成功后自动关闭管道写端，避免用户程序及其后代继承该
                                            fd，（用户代码可以继续继承子进程的文件描述符表，往管道里写入东西，导致 Run Failed）*/
        std::perror("pipe");
        return {RunStatus::InternalError, "", getElapsedUs(), peakMemoryBytes};
    }

    if (!createCgroup(cgroupPath, memoryLimitMiB * 1024LL * 1024)) {
        close(pipeFd[0]);
        close(pipeFd[1]);
        return {RunStatus::InternalError, "", getElapsedUs(), peakMemoryBytes};
    }

    std::string cgroupProcsPath = cgroupPath + "/cgroup.procs";

    pid_t pid = fork();
    if (pid == -1) {
        std::perror("fork");
        close(pipeFd[0]);
        close(pipeFd[1]);
        removeCgroup(cgroupPath);
        return {RunStatus::InternalError, "", getElapsedUs(), peakMemoryBytes};
    }

    if (pid == 0) {
        close(pipeFd[0]); // 子进程关读

        auto childFail = [&]() {
            char errorFlag = 1;
            write(pipeFd[1], &errorFlag, sizeof(errorFlag));
            _exit(1);
        };

        if (!joinCgroup(cgroupProcsPath)) childFail();

        // dup2 重定向输入输出
        int inputFd = open(inputPath.c_str(), O_RDONLY);
        if (inputFd == -1) childFail();
        if (dup2(inputFd, STDIN_FILENO) == -1) childFail();
        close(inputFd);

        int actualOutputFd = open(actualOutputPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (actualOutputFd == -1) childFail();
        if (dup2(actualOutputFd, STDOUT_FILENO) == -1) childFail();
        close(actualOutputFd);

        char *argv[] = {const_cast<char *>(exePath.c_str()), nullptr};
        if (execv(argv[0], argv) == -1) childFail();
    }
    close(pipeFd[1]); // 父进程关写，不关的话，下面的 read() 一直会阻塞，因为还有写端开着

    int sta; // waitpid 写入子进程结束状态；正常退出 WIFEXITED / WEXITSTATUS；信号终止 WIFSIGNALED / WTERMSIG
    bool timeOut = false, coreDump = false;
    long long cpuUsageUs = 0;

    auto terminateAndReap = [&]() -> bool {
        if (!killCgroup(cgroupPath)) {
            if (kill(pid, SIGKILL) == -1) { // cgroup 整组终止失败，至少兜底终止直接子进程，best effort
                std::perror("kill");
                return false;
            }
        }
        if (waitpid(pid, &sta, 0) == -1) { // 回收被杀死的子进程，防止僵尸进程，读取signal终止状态信息
            std::perror("waitpid");
            return false;
        }
        return true;
    };

    while (1) {
        pid_t waitpidResult = waitpid(pid, &sta, WNOHANG);

        if (waitpidResult == -1) {
            std::perror("waitpid");
            close(pipeFd[0]);
            terminateAndReap();
            removeCgroup(cgroupPath);
            return {RunStatus::InternalError, "", getElapsedUs(), peakMemoryBytes};
        }

        coreDump = coreDump || isCoreDumping(pid); // C++短路，一旦检测到 CoreDumping，保持状态。

        if (!readCpuUsage(cgroupPath, cpuUsageUs)) {
            close(pipeFd[0]);
            terminateAndReap();
            removeCgroup(cgroupPath);
            return {RunStatus::InternalError, "", getElapsedUs(), peakMemoryBytes};
        }

        if (waitpidResult > 0) {
            break;
        }

        long long wallElapsedUs = getElapsedUs();
        bool cpuTimeExceeded = cpuUsageUs > timeLimitMs * 1000;
        bool wallTimeExceeded = wallElapsedUs > timeLimitMs * 3000;

        if ((cpuTimeExceeded || wallTimeExceeded) && !coreDump) {
            timeOut = true;
            if (!terminateAndReap()) {
                removeCgroup(cgroupPath);
                return {RunStatus::InternalError, "", cpuUsageUs, peakMemoryBytes};
            }
            break;
        }
        usleep(3000);
    }

    // std::cout << coreDump << '\n';

    char errorFlag;
    ssize_t byteRead = read(pipeFd[0], &errorFlag, sizeof(errorFlag)); // 返回值是实际读到了多少字节（注意不是元素个数），0 = EOF
    close(pipeFd[0]);

    if (byteRead == -1) {
        std::perror("read");
        killCgroup(cgroupPath);
        removeCgroup(cgroupPath);
        return {RunStatus::InternalError, "", cpuUsageUs, peakMemoryBytes};
    }

    long long oomCnt = 0ll;

    if (!readOomKillCount(cgroupPath, oomCnt)) {
        killCgroup(cgroupPath);
        removeCgroup(cgroupPath);
        return {RunStatus::InternalError, "", cpuUsageUs, peakMemoryBytes};
    }

    if (!readMemoryPeak(cgroupPath, peakMemoryBytes)) {
        killCgroup(cgroupPath);
        removeCgroup(cgroupPath);
        return {RunStatus::InternalError, "", cpuUsageUs, peakMemoryBytes};
    }

    if (!killCgroup(cgroupPath)) {
        removeCgroup(cgroupPath);
        return {RunStatus::InternalError, "", cpuUsageUs, peakMemoryBytes};
    }

    if (!removeCgroup(cgroupPath)) {
        return {RunStatus::InternalError, "", cpuUsageUs, peakMemoryBytes};
    }

    // byteRead > 0 表示子进程在 exec 前发生 MiniJudge 内部错误
    if (byteRead > 0) {
        return {RunStatus::InternalError, "", cpuUsageUs, 0ll};
    }

    if (oomCnt > 0) {
        return {RunStatus::MemoryLimitExceeded, "", cpuUsageUs, peakMemoryBytes};
    }

    if (timeOut) {
        return {RunStatus::TimeLimitExceeded, "", cpuUsageUs, peakMemoryBytes};
    }

    if (WIFSIGNALED(sta)) {
        return {RunStatus::RuntimeError, "", cpuUsageUs, peakMemoryBytes};
    }

    // 只剩下用户代码正常退出的情况了，那就看 return 的值（也就是退出码）是不是 0 了。是 0 就 OK，否则 RE
    if (WIFEXITED(sta) && (WEXITSTATUS(sta) != 0)) { // 进程正常退出，但退出码非 0;
        return {RunStatus::RuntimeError, "", cpuUsageUs, peakMemoryBytes};
    }
    if (WIFEXITED(sta) && (WEXITSTATUS(sta) == 0)) {
        RunResult compareResult = compare(actualOutputPath, expectedPath);
        compareResult.memoryBytes = peakMemoryBytes;
        compareResult.timeUs = cpuUsageUs;
        return compareResult;
    }

    return {RunStatus::InternalError, "", cpuUsageUs, peakMemoryBytes};
}
