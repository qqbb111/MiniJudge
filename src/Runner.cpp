#include "Runner.h"
#include <string>
#include <chrono>

#include <unistd.h> // fork, dup2, close, execv, _exit, pipe
#include <sys/wait.h> // waitpid, WIFEXITED, WEXITSTATUS
#include <fcntl.h> // open, O_RDONLY...
#include <cstdio> // perror
#include <signal.h> // SIGKILL

RunResult run(const std::string& exePath, const std::string& inputPath, const std::string& actualOutputPath){
    auto start = std::chrono::steady_clock::now();
    auto getElapsedUs = [&start](){
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
    };

    int pipeFd[2]; // fork 之前创建管道，供父子进程间通信
    if(pipe(pipeFd) == -1){
        std::perror("pipe");
        return {RunStatus::InternalError, getElapsedUs()};
    }

    pid_t pid = fork();
    if(pid == -1){
        std::perror("fork");
        close(pipeFd[0]);
        close(pipeFd[1]);
        return {RunStatus::InternalError, getElapsedUs()};
    }

    if(pid == 0){
        close(pipeFd[0]); // 子进程关读

        auto childFail = [&](const char* message){
            std::perror(message);
            char errorFlag = 1;
            write(pipeFd[1], &errorFlag, sizeof(errorFlag));
            _exit(1);
        };

        // dup2 重定向输入输出
        int inputFd = open(inputPath.c_str(), O_RDONLY);
        if(inputFd == -1){
            childFail("open input file");
        }
        if(dup2(inputFd, STDIN_FILENO) == -1){
            childFail("dup2 input file");
        }
        close(inputFd);

        int actualOutputFd = open(actualOutputPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if(actualOutputFd == -1){
            childFail("open actual output file");
        }
        if(dup2(actualOutputFd, STDOUT_FILENO) == -1){
            childFail("dup2 actual output file");
        }
        close(actualOutputFd);

        std::string program = exePath;
        char* argv[] = {program.data(), nullptr};
        if(execv(program.c_str(), argv) == -1){
            childFail("execv");
        }

    }
    close(pipeFd[1]); // 父进程关写

    int sta; // waitpid 写入子进程结束状态；正常退出 WIFEXITED / WEXITSTATUS；信号终止 WIFSIGNALED / WTERMSIG
    bool timeOut = false;
    while(1){
        pid_t waitpidResult = waitpid(pid, &sta, WNOHANG);
        if(waitpidResult == -1){
            std::perror("waitpid");
            close(pipeFd[0]);
            return {RunStatus::InternalError, getElapsedUs()};
        }
        if(waitpidResult > 0){
            break;
        }
        long long elapsedUs = getElapsedUs();
        if(elapsedUs > 1000 * 1000){
            kill(pid, SIGKILL);
            waitpid(pid, &sta, 0); // 回收被杀死的子进程，防止僵尸进程，读取signal终止状态信息
            timeOut = true;
            break;
        }
        usleep(1000);
    }
    
    char errorFlag;
    ssize_t byteRead = read(pipeFd[0], &errorFlag, sizeof(errorFlag));
    close(pipeFd[0]);

    if(byteRead == -1){
        std::perror("read");
        return {RunStatus::InternalError, getElapsedUs()};
    }
    
    // 这种分法就是看是不是 MiniJudge 自己的问题，自己的问题肯定只有 byteRead > 0
    if(byteRead > 0){
        return {RunStatus::InternalError, getElapsedUs()};
    }
    
    if(timeOut){
        return {RunStatus::TimeLimitExceeded, getElapsedUs()};
    } 
    
    if(WIFSIGNALED(sta)){ // 用户程序因信号停止，判 RE / TLE
        return {RunStatus::RuntimeError, getElapsedUs()};
    }

    // 只剩下用户代码正常退出的情况了，那就看 return 的值（也就是退出码）是不是 0 了。是 0 就 OK，否则 RE
    if(WIFEXITED(sta) && (WEXITSTATUS(sta) != 0)){ // 进程正常退出，但退出码非 0;
        return {RunStatus::RuntimeError, getElapsedUs()};
    }
    if(WIFEXITED(sta) && (WEXITSTATUS(sta) == 0)){
        return {RunStatus::Ok, getElapsedUs()};
    }

    return {RunStatus::InternalError, getElapsedUs()};
}
