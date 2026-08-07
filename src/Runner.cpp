#include "Runner.h"
#include <string>
#include <chrono>

#include <unistd.h> // fork, dup2, close, execv, _exit
#include <sys/wait.h> // waitpid, WIFEXITED, WEXITSTATUS
#include <fcntl.h> // open, O_RDONLY...
#include <cstdio> // perror


RunResult run(const std::string& exePath, const std::string& inputPath, const std::string& actualOutputPath){
    auto start = std::chrono::steady_clock::now();
    pid_t pid = fork();
    if(pid == -1){
        std::perror("fork");
        auto end = std::chrono::steady_clock::now();
        long long elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        return {false, elapsedUs};
    }

    if(pid == 0){
        int inputFd = open(inputPath.c_str(), O_RDONLY);
        if(inputFd == -1){
            std::perror("open input file");
            _exit(1);
        }
        if(dup2(inputFd, STDIN_FILENO) == -1){
            std::perror("dup2 input file");
            close(inputFd);
            _exit(1);
        }
        close(inputFd);

        int actualOutputFd = open(actualOutputPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if(actualOutputFd == -1){
            std::perror("open actualOutput file");
            _exit(1);
        }
        if(dup2(actualOutputFd, STDOUT_FILENO) == -1){
            std::perror("dup2 actualOutput file");
            close(actualOutputFd);
            _exit(1);
        }
        close(actualOutputFd);

        std::string program = exePath;
        char* argv[] = {program.data(), nullptr};

        execv(exePath.c_str(), argv);

        std::perror("execv");
        _exit(1);
    }
    int sta;
    if(waitpid(pid, &sta, 0) == -1){
        std::perror("waitpid");
        auto end = std::chrono::steady_clock::now();
        long long elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        return {false, elapsedUs};
    }
    auto end = std::chrono::steady_clock::now();
    long long elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    bool success = (WIFEXITED(sta) && WEXITSTATUS(sta) == 0);
    return {success, elapsedUs};
}
