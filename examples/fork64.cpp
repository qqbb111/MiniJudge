#include <iostream>
#include <vector>

#include <unistd.h>   // fork, pipe, read, close, _exit
#include <sys/wait.h> // waitpid
#include <cstdio>     // perror

int main() {
    int pipeFd[2];

    if (pipe(pipeFd) == -1) {
        perror("pipe");
        return 1;
    }

    std::vector<pid_t> children;

    for (int i = 1; i <= 70; ++i) {
        pid_t pid = fork();

        if (pid == -1) {
            std::cerr << "fork #" << i << " failed: ";
            perror("fork");
            break;
        }

        if (pid == 0) {
            // child 不允许继续执行 for，也就不会继续 fork
            close(pipeFd[1]);

            // parent 不关闭写端之前，这里一直阻塞
            char c;
            read(pipeFd[0], &c, 1);

            close(pipeFd[0]);
            _exit(0);
        }

        // 只有 parent 能走到这里
        children.push_back(pid);
    }

    std::cerr << "Successfully created " << children.size() << " children\n";

    // parent 关闭写端
    // 所有 child 的 read() 得到 EOF，然后退出
    close(pipeFd[1]);
    close(pipeFd[0]);

    for (pid_t pid : children) {
        waitpid(pid, nullptr, 0);
    }

    int a, b; std::cin >> a >> b;
    std::cout << a + b << std::endl;

    return 0;
}
