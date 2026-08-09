# MiniJudge 学习笔记

> 只记录已经在项目中实际遇到、理解并验证过的知识。

# 1. 当前评测流程

```text
读取源码路径
    ↓
扫描并校验测试点
    ↓
编译用户源码
    ↓
fork 创建子进程
    ↓
open + dup2 重定向输入输出
    ↓
execv 执行用户程序
    ↓
父进程 waitpid 获取结束状态
    ↓
判断 OK / RE / Run failed
    ↓
OK 才进入 compare
    ↓
AC / WA
```

当前结果：

```text
AC          输出正确
WA          输出错误
CE          编译错误
RE          用户程序运行错误
Run failed  MiniJudge 自身运行流程失败
```

---

# 2. C++ 基础

## 2.1 `const T&`

只读的大对象通常使用常量引用：

```cpp
bool run(const std::string& path);
```

```text
T        复制对象
T&       引用，可以修改
const T& 引用，不可修改
```

结论：避免复制，同时防止误修改。

常见坑：`int`、`char` 等小型基础类型通常直接值传递。

---

## 2.2 `std::` 与头文件

`std` 是 C++ 标准库命名空间：

```cpp
std::string
std::cout
std::vector
```

工程代码优先显式写 `std::`，头文件中不要写：

```cpp
using namespace std;
```

按实际依赖包含头文件，不使用：

```cpp
#include <bits/stdc++.h>
```

常见坑：代码能编译不代表头文件依赖正确，可能只是被其他头文件间接包含。

---

## 2.3 `c_str()` 与 `data()`

传统 C 接口常需要字符指针：

```cpp
std::string path = "tmp/user_program";
open(path.c_str(), O_RDONLY);
```

`c_str()` 返回 `const char*`。

C++17 中，非 `const std::string` 的 `data()` 可得到可修改的字符指针，因此当前 `execv()` 参数数组使用：

```cpp
std::string program = exePath;
char* argv[] = {program.data(), nullptr};
execv(program.c_str(), argv);
```

常见坑：字符串被修改或销毁后，之前取得的内部指针可能失效。

---

## 2.4 结构体保存一组结果

当前：

```cpp
struct RunResult {
    int status; // 0 -> OK, 1 -> RE, 2 -> Run failed
    long long elapsedMicroseconds;
};
```

作用：一次返回运行状态和耗时。

聚合初始化：

```cpp
return {1, elapsedUs};
```

当前状态较少，先使用整数；如果结果类型继续增加，再考虑 `enum class`。

---

## 2.5 Lambda 基础

Lambda 是定义在当前位置的小函数：

```cpp
auto f = [捕获](参数) {
    // 函数体
};
```

常见捕获：

```text
[]     不捕获外部局部变量
[&]    按引用捕获需要的外部变量
[=]    按值捕获需要的外部变量
[&x]   只按引用捕获 x
[x]    只按值捕获 x
```

当前 Runner：

```cpp
auto getElapsedUs = [&start]() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start
    ).count();
};
```

用于消除重复计时代码。

子进程错误处理：

```cpp
auto childFail = [&](const char* message) {
    std::perror(message);
    char errorFlag = 1;
    write(pipeFd[1], &errorFlag, sizeof(errorFlag));
    _exit(1);
};
```

结论：只在当前函数内有意义的少量重复逻辑，可以使用局部 lambda，不必额外拆模块。

---

# 3. 多文件工程

## 3.1 `.h` 与 `.cpp`

```text
.h    声明模块提供什么
.cpp  实现模块具体怎么做
```

头文件：

```cpp
#pragma once
```

防止一次编译中重复包含。

项目按职责拆分，不采用“一函数一个 `.h/.cpp`”。

当前模块：

```text
Compiler          编译用户源码
Runner            运行、重定向、进程管理、计时
Checker           比较输出
TestCasesFinder   发现并校验测试数据
main              组织完整流程
```

---

## 3.2 CMake

配置：

```bash
cmake -S . -B build
```

构建：

```bash
cmake --build build
```

当前目标：

```cmake
add_executable(
    minijudge
    src/main.cpp
    src/Compiler.cpp
    src/Runner.cpp
    src/Checker.cpp
    src/TestCasesFinder.cpp
)
```

头文件目录：

```cmake
target_include_directories(minijudge PRIVATE include)
```

警告选项：

```cmake
-Wall -Wextra -Wpedantic
```

常见坑：新增 `.cpp` 却没有加入 `CMakeLists.txt`，可能产生 `undefined reference`。

---

# 4. 命令行参数

入口：

```cpp
int main(int argc, char* argv[])
```

运行：

```bash
./build/minijudge examples/accepted.cpp
```

对应：

```text
argv[0]  MiniJudge 启动路径
argv[1]  用户源码路径
```

使用前先检查：

```cpp
if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <source_path>\n";
    return 1;
}
```

常见坑：检查 `argc` 前访问 `argv[1]`。

---

# 5. `std::filesystem` 与测试点

## 5.1 路径检查与遍历

```cpp
std::filesystem::exists(path);
std::filesystem::is_directory(path);
```

遍历：

```cpp
for (const auto& entry :
     std::filesystem::directory_iterator(testDir)) {
    if (!entry.is_regular_file()) {
        continue;
    }
}
```

目录遍历顺序没有保证。

---

## 5.2 `path`

```cpp
std::filesystem::path path = entry.path();

path.filename().string();
path.stem().string();
path.extension().string();
```

拼接：

```cpp
std::filesystem::path input =
    std::filesystem::path(testDir) / (name + ".in");
```

常见坑：相对路径基于程序启动时的当前工作目录，不是可执行文件所在目录。

当前必须从项目根目录运行。

---

## 5.3 测试点配对

规则：

```text
<name>.in + <name>.out = 一个测试点
```

使用两个集合：

```cpp
std::set<std::string> inputNames;
std::set<std::string> outputNames;
```

必须双向检查：

```text
.in 是否缺 .out
.out 是否缺 .in
```

不能仅通过文件数量判断。

当前测试点按 `std::set<std::string>` 的字符串字典序运行。

---

## 5.4 测试名白名单

当前允许：

```text
A-Z a-z 0-9 _ - # .
```

当前 `Checker` 仍通过 Shell 调用 `diff`，因此继续限制测试名，避免路径中的特殊字符改变命令结构。

`.in`、`.out` 这种空测试名需要单独检查。

---

# 6. `std::system()` 与 Shell

`std::system()` 让 Shell 执行命令：

```cpp
std::string cmd = "ls";
int res = std::system(cmd.c_str());
```

早期 Runner：

```bash
./program < test.in > actual.out
```

Shell 自动完成：

```text
启动程序
输入重定向
输出重定向
等待程序结束
```

当前状态：

```text
Runner    已移除 system()
Compiler  仍使用 system() 调用 g++
Checker   仍使用 system() 调用 diff
```

结论：改用 Linux 进程接口后，原来 Shell 自动做的工作需要 MiniJudge 自己完成。

---

# 7. Linux 标准输入输出与文件描述符

默认文件描述符：

```text
0  stdin
1  stdout
2  stderr
```

`std::cin` 从标准输入读取，`std::cout` 写向标准输出。

Shell：

```bash
program < input.txt > output.txt
```

当前 Runner 使用 `open()` + `dup2()` 实现相同效果。

---

# 8. `open()`

输入：

```cpp
int inputFd = open(inputPath.c_str(), O_RDONLY);
```

成功返回非负文件描述符，失败返回 `-1`。

输出：

```cpp
int outputFd = open(
    outputPath.c_str(),
    O_WRONLY | O_CREAT | O_TRUNC,
    0644
);
```

```text
O_WRONLY  只写
O_CREAT   不存在则创建
O_TRUNC   已存在则清空
0644      所有者读写，同组和其他用户只读
```

`0644` 中开头的 `0` 表示八进制：

```text
r = 4
w = 2
x = 1
```

常见坑：使用 `O_CREAT` 时需要提供创建权限参数。

---

# 9. `dup2()`

```cpp
dup2(oldFd, newFd);
```

作用：让 `newFd` 指向 `oldFd` 当前指向的对象。

输入重定向：

```cpp
dup2(inputFd, STDIN_FILENO);
```

输出重定向：

```cpp
dup2(outputFd, STDOUT_FILENO);
```

执行后：

```text
0 -> inputPath
1 -> actualOutputPath
```

之后 `execv()` 启动的用户程序仍然使用自己的 `cin/cout`，但实际连接到测试文件和输出文件。

常见坑：输出必须使用 `STDOUT_FILENO`，不能复制输入重定向代码后忘记修改。

---

# 10. `close()` 与 fd 生命周期

原则：

> fd 最后一次使用结束后立即关闭。

例如：

```cpp
dup2(inputFd, STDIN_FILENO);
close(inputFd);
```

`dup2()` 后标准输入已经保留了对文件的引用，原始 `inputFd` 不再需要。

父进程提前 `return` 前，应关闭仍然持有的 fd。

子进程马上 `_exit()` 时，不必为了防泄漏逐个关闭剩余 fd，进程结束时内核会回收。

---

# 11. `fork()`

```cpp
pid_t pid = fork();
```

返回：

```text
pid < 0   创建失败
pid == 0  当前执行流是子进程
pid > 0   当前执行流是父进程，值为子进程 PID
```

`fork()` 后父子进程从下一条语句继续执行。

父子进程拥有独立地址空间。

典型模型：

```text
父进程 fork
├─ child：准备环境，exec 目标程序
└─ parent：保留下来监督 child
```

`fork()` 只负责创建子进程；等待、获取状态、通信等由其他接口完成。

---

# 12. `execv()`

```cpp
execv(program.c_str(), argv);
```

作用：

> 用新的程序替换当前进程正在执行的程序。

关键结论：

- 不创建新进程
- PID 不变
- 成功后不会返回
- 失败返回 `-1`

当前流程：

```text
fork
↓
子进程 open / dup2
↓
execv
↓
同一个子进程变成 user_program
```

---

# 13. `_exit()`

子进程在 `execv()` 前出现错误时：

```cpp
_exit(1);
```

作用：立即结束当前子进程。

在 `run()` 中不能简单 `return`，否则只是从 `run()` 返回，子进程还可能继续执行 MiniJudge 后续逻辑。

注意：

```text
return / exit / _exit
```

从父进程 `waitpid()` 看，都属于正常退出方式，`WIFEXITED(status)` 为真。

---

# 14. `waitpid()`

```cpp
int status;
waitpid(childPid, &status, 0);
```

当前用途：

- 等待指定子进程结束
- 获取子进程结束状态
- 回收子进程

正常退出：

```cpp
WIFEXITED(status)
WEXITSTATUS(status)
```

例如：

```text
return 0  -> WIFEXITED=true, exitCode=0
return 1  -> WIFEXITED=true, exitCode=1
_exit(1)  -> WIFEXITED=true, exitCode=1
```

`waitpid()` 无法仅凭退出码区分：

```text
MiniJudge 在 execv 前 _exit(1)
用户程序 return/exit 1
```

这正是当前 Runner 使用 `pipe()` 的原因。

---

# 15. Signal 与 RE

Signal 是 Linux 向进程通知事件的一种机制。

例如非法内存访问：

```text
用户程序访问非法地址
↓
内核产生 SIGSEGV
↓
默认动作终止进程
↓
父进程 waitpid 得到信号终止状态
```

判断：

```cpp
WIFSIGNALED(status)
WTERMSIG(status)
```

Linux 上常见的 `SIGSEGV` 信号编号为 11。

当前判定：

```text
WIFSIGNALED                 -> RE
WIFEXITED && exitCode != 0  -> RE
WIFEXITED && exitCode == 0  -> OK
```

常见坑：不要把 `WIFEXITED` 理解成“退出码为 0”；它只表示进程通过正常退出路径结束。

---

# 16. `pipe()`：父子进程通信

创建：

```cpp
int pipeFd[2];
pipe(pipeFd);
```

```text
pipeFd[0]  读端
pipeFd[1]  写端
```

必须在 `fork()` 前创建，使父子进程都继承这组 fd。

当前方向：

```text
子进程 --错误标记--> 父进程
```

子进程在 `open / dup2 / execv` 失败时：

```cpp
char errorFlag = 1;
write(pipeFd[1], &errorFlag, sizeof(errorFlag));
_exit(1);
```

父进程：

```cpp
char errorFlag;
ssize_t byteRead =
    read(pipeFd[0], &errorFlag, sizeof(errorFlag));
```

返回：

```text
byteRead > 0   收到错误标记
byteRead == 0  没有数据且写端已关闭
byteRead == -1 read 失败
```

普通变量不能跨 `fork()` 把子进程修改传回父进程，pipe 可以。

---

# 17. Runner 当前结果分类

当前 `RunResult.status`：

```text
0  OK
1  RE
2  Run failed
```

判断逻辑：

```text
pipe / fork / waitpid / read 自身失败
→ Run failed

pipe 收到子进程启动错误标记
→ Run failed

pipe 无启动错误 + WIFSIGNALED
→ RE

pipe 无启动错误 + WIFEXITED + exitCode != 0
→ RE

pipe 无启动错误 + WIFEXITED + exitCode == 0
→ OK
```

只有 `OK` 才进入 `Checker`：

```text
compare 相同 -> AC
compare 不同 -> WA
```

---

# 18. `perror()` 与 `std::cerr`

`std::cerr`：

```cpp
std::cerr << "Missing source path\n";
```

用于输出自己定义的错误信息。

`perror()`：

```cpp
std::perror("execv");
```

会根据当前 `errno` 自动补充系统错误原因。

结论：

```text
系统调用刚失败 -> perror
业务/逻辑错误  -> cerr
```

常见坑：系统调用失败后应尽快调用 `perror()`，中间其他调用可能改变 `errno`。

---

# 19. Linux 系统接口头文件

当前常用：

```cpp
#include <unistd.h>    // fork, execv, dup2, close, pipe, read, write, _exit
#include <sys/wait.h>  // waitpid, WIFEXITED, WEXITSTATUS, WIFSIGNALED
#include <fcntl.h>     // open, O_RDONLY, O_WRONLY...
#include <cstdio>      // perror
```

不必死记所有头文件；忘记时查：

```bash
man 2 fork
man 2 waitpid
man 3 execv
```

原则：使用哪个接口，就直接包含声明该接口的头文件，不依赖间接包含。

---

# 20. 运行时间统计

使用：

```cpp
std::chrono::steady_clock
```

适合测量时间间隔，不受系统日期时间调整影响。

当前内部保存微秒：

```cpp
std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now() - start
).count();
```

输出：

```cpp
std::cout << std::fixed << std::setprecision(3);
std::cout << elapsedMicroseconds / 1000.0 << " ms";
```

当前测量的是墙上时间，包含进程创建、文件重定向、程序执行、调度与等待等开销。

不要通过“空跑一次再减基线”修正时间，因为调度、缓存等开销并不固定。

---

# 21. Ubuntu Apport 与信号型 RE 计时

当前 Ubuntu 的 `core_pattern` 使用 Apport crash handler。

用户程序 `SIGSEGV` 后，Ubuntu 可能额外启动崩溃收集流程，导致：

```text
用户程序实际很快崩溃
但 waitpid 数秒后才返回
```

因此部分 RE 的墙上时间可能明显偏大。

结论：

- RE 判定本身仍然正确
- 不应让 MiniJudge 擅自修改系统全局 `core_pattern`
- 设计 TLE 时不能单纯以最终墙上时间超过阈值就直接判 TLE

---

# 22. `diff` 与 Checker

当前：

```bash
diff -wB actual.out expected.out
```

```text
-w  忽略空白字符差异
-B  忽略空白行
```

返回：

```text
0   相同
1   不同
>1  diff 自身执行错误
```

当前 `Checker` 仍通过 `std::system()` 调用 `diff`。

---

# 23. Git

基本流程：

```text
工作区
↓ git add
暂存区
↓ git commit
本地仓库
↓ git push
远程仓库
```

常用：

```bash
git status
git diff
git log --oneline
git add <files>
git commit -m "type: description"
git push
```

常见提交类型：

```text
feat      新功能
fix       修复
docs      文档
refactor  重构
```

修改最近一次尚未推送的提交：

```bash
git commit --amend --no-edit
```

撤回最近提交但保留代码：

```bash
git reset --soft HEAD~1
```

常见坑：不要随意对已经 push 的提交重写历史。

---

# 24. VS Code Remote Git 凭证通道

Remote-SSH 终端可能保存：

```text
VSCODE_GIT_IPC_HANDLE
```

如果 VS Code Git 后台重启，旧终端可能仍指向旧 socket，导致：

```text
ECONNREFUSED ... vscode-git-xxxx.sock
```

新建终端后会继承新的环境变量。

结论：出现这类错误时先确认 socket 是否过期，不要直接重配 `user.name` / `user.email`。

---

# 25. 当前状态

## 已完成

- 基础编译、运行、答案比较
- AC / WA / CE
- CMake 构建
- 命令行源码参数
- 自动扫描并校验测试数据
- 字符串测试点名称
- 每个测试点运行时间统计
- Runner 从 `std::system()` 替换为 Linux 进程接口
- `fork + open + dup2 + execv + waitpid`
- pipe 父子进程通信
- 区分 `RE` 与 `Run failed`
- 信号终止和非零退出的 RE 判断

## 尚未完成

- TLE 与超时进程终止
- `Compiler` 去除 `std::system()`
- `Checker` 去除 `std::system()`
- 内存和其他资源限制
- 修复必须从项目根目录运行的问题
- 结构化评测报告

## 当前唯一下一步

学习并验证：

```text
waitpid(..., WNOHANG)
```

目标：为 TLE 的非阻塞等待和超时控制建立运行模型。

## 明确暂缓

- Compiler / Checker 重构
- 内存限制
- 并行评测
- 复杂报告格式
