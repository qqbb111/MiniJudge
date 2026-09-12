#include "Compiler.h"
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdio>
#include <string>

bool compile(const std::string &codePath, const std::string &exePath, const std::string &logPath) { // 编译
    pid_t pid = fork();
    if (pid == -1) {
        std::perror("fork");
        return false;
    }

    if (pid == 0) {
        auto childFail = [&](const char *message) {
            std::perror(message);
            _exit(1);
        };

        // dup2 重定向输出
        int logFd = open(logPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (logFd == -1) {
            childFail("open compile.log");
        }
        if (dup2(logFd, STDERR_FILENO) == -1) {
            childFail("dup2 compile.log");
        }
        close(logFd);

        char *args[] = {const_cast<char *>("g++"),
                        const_cast<char *>(codePath.c_str()),
                        const_cast<char *>("-std=c++17"),
                        const_cast<char *>("-O2"),
                        const_cast<char *>("-o"),
                        const_cast<char *>(exePath.c_str()),
                        nullptr};

        if (execvp(args[0], args) == -1) {
            childFail("execvp");
        }
    }
    int sta; // waitpid 写入子进程结束状态；正常退出 WIFEXITED / WEXITSTATUS；信号终止 WIFSIGNALED / WTERMSIG

    if (waitpid(pid, &sta, 0) == -1) {
        perror("waitpid");
        return false;
    }

    if (WIFEXITED(sta) && (WEXITSTATUS(sta) == 0)) {
        return true;
    }
    return false;
}
