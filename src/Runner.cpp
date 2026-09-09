#include "Runner.h"
#include "Cgroup.h"

#include <string>
#include <chrono>
#include <unistd.h>       // fork, dup2, close, execv, _exit, pipe
#include <sys/wait.h>     // waitpid, waitpid, WIFEXITED, WEXITSTATUS
#include <sys/resource.h> // rlimit, setrlimit, RLIMIT_AS
#include <fcntl.h>        // open, O_RDONLY...
#include <cstdio>         // perror
#include <signal.h>       // SIGKILL
#include <fstream>        // ifstream
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

RunResult run(const std::string &exePath, const std::string &inputPath, const std::string &actualOutputPath, long long timeLimitMs, long long memoryLimitMiB) {
    auto start = std::chrono::steady_clock::now();
    auto getElapsedUs = [&start]() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count(); };

    long long peakMemoryBytes = 0ll;

    int pipeFd[2];                        // fork 之前创建管道，供父子进程间通信
    if (pipe2(pipeFd, O_CLOEXEC) == -1) { // 如果不用 pipe2 的 O_CLOEXEC 参数（Close On Exec），用户代码可以继续继承子进程的文件描述符表，往管道里写入东西，导致 Run Failed
        std::perror("pipe");
        return {RunStatus::InternalError, getElapsedUs(), peakMemoryBytes};
    }
    std::string cgroupPath = "/sys/fs/cgroup/minijudge/run";
    if (!createCgroup(cgroupPath, memoryLimitMiB * 1024LL * 1024)) {
        close(pipeFd[0]);
        close(pipeFd[1]);
        return {RunStatus::InternalError, getElapsedUs(), peakMemoryBytes};
    }

    pid_t pid = fork();
    if (pid == -1) {
        std::perror("fork");
        close(pipeFd[0]);
        close(pipeFd[1]);
        removeCgroup(cgroupPath);
        return {RunStatus::InternalError, getElapsedUs(), peakMemoryBytes};
    }

    if (pid == 0) {
        close(pipeFd[0]); // 子进程关读

        auto childFail = [&](const char *message) {
            std::perror(message);
            char errorFlag = 1;
            write(pipeFd[1], &errorFlag, sizeof(errorFlag));
            _exit(1);
        };

        if (!joinCgroup(cgroupPath)) {
            char errorFlag = 1;
            write(pipeFd[1], &errorFlag, sizeof(errorFlag));
            _exit(1);
        }

        // dup2 重定向输入输出
        int inputFd = open(inputPath.c_str(), O_RDONLY);
        if (inputFd == -1) {
            childFail("open input file");
        }
        if (dup2(inputFd, STDIN_FILENO) == -1) {
            childFail("dup2 input file");
        }
        close(inputFd);

        int actualOutputFd = open(actualOutputPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (actualOutputFd == -1) {
            childFail("open actual output file");
        }
        if (dup2(actualOutputFd, STDOUT_FILENO) == -1) {
            childFail("dup2 actual output file");
        }
        close(actualOutputFd);

        std::string program = exePath;
        char *argv[] = {program.data(), nullptr};
        if (execv(program.c_str(), argv) == -1) {
            childFail("execv");
        }
    }
    close(pipeFd[1]); // 父进程关写，不关的话，下面的 read() 一直会阻塞，因为还有写端开着

    int sta; // waitpid 写入子进程结束状态；正常退出 WIFEXITED / WEXITSTATUS；信号终止 WIFSIGNALED / WTERMSIG
    bool timeOut = false, coreDump = false;
    while (1) {
        pid_t waitpidResult = waitpid(pid, &sta, WNOHANG);
        if (waitpidResult == -1) {
            std::perror("waitpid");
            close(pipeFd[0]);
            removeCgroup(cgroupPath);
            return {RunStatus::InternalError, getElapsedUs(), peakMemoryBytes};
        }
        coreDump = coreDump || isCoreDumping(pid); // C++短路，一旦检测到 CoreDumping，保持状态。
        if (waitpidResult > 0) {
            break;
        }
        long long elapsedUs = getElapsedUs();
        if (elapsedUs > timeLimitMs * 1000 && !coreDump) {
            kill(pid, SIGKILL);
            timeOut = true;
            waitpid(pid, &sta, 0); // 回收被杀死的子进程，防止僵尸进程，读取signal终止状态信息
            break;
        }
        usleep(1000);
    }

    // std::cout << coreDump << '\n';

    char errorFlag;
    ssize_t byteRead = read(pipeFd[0], &errorFlag, sizeof(errorFlag)); // 返回值是实际读到了多少字节（注意不是元素个数）
    close(pipeFd[0]);

    long long oomCnt = 0ll;

    if (!readOomKillCount(cgroupPath, oomCnt)) {
        removeCgroup(cgroupPath);
        return {RunStatus::InternalError, getElapsedUs(), peakMemoryBytes};
    }

    if (!readMemoryPeak(cgroupPath, peakMemoryBytes)) {
        removeCgroup(cgroupPath);
        return {RunStatus::InternalError, getElapsedUs(), peakMemoryBytes};
    }

    if (!removeCgroup(cgroupPath)) {
        return {RunStatus::InternalError, getElapsedUs(), peakMemoryBytes};
    }

    // 这种分法就是看是不是 MiniJudge 自己的问题，自己的问题肯定只有 byteRead > 0
    if (byteRead > 0) {
        return {RunStatus::InternalError, getElapsedUs(), 0ll};
    }

    if (oomCnt > 0) return {RunStatus::MemoryLimitExceeded, getElapsedUs(), peakMemoryBytes};

    if (WIFSIGNALED(sta)) { // 用户程序因信号停止，判 RE / TLE
        int sig = WTERMSIG(sta);
        if (timeOut && sig == SIGKILL) return {RunStatus::TimeLimitExceeded, getElapsedUs(), peakMemoryBytes};
        return {RunStatus::RuntimeError, getElapsedUs(), peakMemoryBytes};
    }

    // 只剩下用户代码正常退出的情况了，那就看 return 的值（也就是退出码）是不是 0 了。是 0 就 OK，否则 RE
    if (WIFEXITED(sta) && (WEXITSTATUS(sta) != 0)) { // 进程正常退出，但退出码非 0;
        return {RunStatus::RuntimeError, getElapsedUs(), peakMemoryBytes};
    }
    if (WIFEXITED(sta) && (WEXITSTATUS(sta) == 0)) {
        return {RunStatus::Ok, getElapsedUs(), peakMemoryBytes};
    }

    return {RunStatus::InternalError, getElapsedUs(), peakMemoryBytes};
}
