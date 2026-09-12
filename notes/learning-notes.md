# MiniJudge 学习笔记

---

# C++ 基础

## 1. `const T&` 传参

只读的 `string`、`vector` 等对象通常使用：

```cpp
bool run(const std::string& path);
```

区别：

```text
T        复制对象
T&       引用，可修改
const T& 引用，不可修改
```

作用：

* 避免复制
* 防止函数修改原对象

---

## 2. `std::string::c_str()`

将 `std::string` 转换为：

```cpp
const char*
```

例如：

```cpp
std::string cmd = "ls";
system(cmd.c_str());
```

常用于传统 C 接口。

---

## 3. `std::string::data()`

C++17 中：

```cpp
std::string program = exePath;
char* argv[] = {program.data(), nullptr};
```

可用于构造 `execv()` 的参数数组。

注意：

```text
argv 最后一项必须是 nullptr
```

---

## 4. `std::string::find()`

查找字符串：

```cpp
auto pos = line.find("CoreDumping:");
```

判断是否找到：

```cpp
if (pos != std::string::npos) {
}
```

`npos` 表示：

```text
not found
```

---

## 5. `size_t`

常用于表示：

```text
长度
大小
字符串位置
容器下标
```

例如：

```cpp
size_t pos = str.find(':');
```

不知道具体返回类型时也可以：

```cpp
auto pos = str.find(':');
```

---

## 6. `substr()`

截取字符串：

```cpp
std::string s = "abc: 123";

auto pos = s.find(':');
std::string value = s.substr(pos + 1);
```

结果：

```text
" 123"
```

---

## 7. `std::stoi()`

将字符串转换为整数：

```cpp
int value = std::stoi("   1");
```

结果：

```text
1
```

---

# Linux 标准输入输出

## 8. 文件描述符

Linux 默认：

```text
0 stdin
1 stdout
2 stderr
```

对应：

```cpp
STDIN_FILENO
STDOUT_FILENO
STDERR_FILENO
```

---

## 9. Shell 重定向

```bash
./program < input.txt > output.txt
```

等价流程：

```text
input.txt
→ stdin
→ program
→ stdout
→ output.txt
```

错误输出：

```bash
2> error.log
```

---

## 10. `/dev/null`

Linux 黑洞文件：

```bash
command > /dev/null
```

写入内容直接丢弃。

---

# system()

## 11. `system()`

让程序调用 Shell 执行命令：

```cpp
std::string cmd = "ls";
int result = system(cmd.c_str());
```

第一版 MiniJudge 使用：

```text
Compiler
Runner
Checker
```

执行外部命令。

缺点：

* 依赖 Shell
* 路径转义复杂
* 无法精确控制子进程
* 不方便实现 RE/TLE

当前 Runner 已改为 Linux 进程接口。

---

# 编译

## 12. compile()

接口：

```cpp
bool compile(
    const std::string& sourcePath,
    const std::string& executablePath
);
```

核心命令：

```bash
g++ source.cpp -std=c++17 -O2 \
-o tmp/user_program \
2> tmp/compile.log
```

失败：

```text
CE
```

常见坑：

不能通过“可执行文件是否存在”判断编译成功，因为旧文件可能残留。

---

# 输出比较

## 13. Checker：移除 `diff` 依赖

Checker 不再通过外部 `diff` 命令比较答案，而是直接使用 C++ 读取输出文件。

当前比较规则：

- 忽略空行；
- 忽略每行末尾的空格、Tab 和 `\r`；
- 行首和行中间的空白仍参与比较；
- 其他内容逐字符一致。

Checker 使用三态结果区分：

```text
Accepted     → 输出一致
WrongAnswer  → 输出不同
Error        → 文件打开或读取失败
```

Checker 的内部错误不能判为 WA，由 `main` 输出 `Judge failed`。

---

# 多文件项目

## 14. `.h` 与 `.cpp`

`.h`：

```text
声明接口
定义公共类型
```

`.cpp`：

```text
实现功能
```

当前按照职责拆分模块，不采用“一函数一个文件”。

---

## 15. `#pragma once`

写在头文件顶部：

```cpp
#pragma once
```

避免头文件重复包含。

---

## 16. `std::`

`std` 是 C++ 标准库命名空间：

```cpp
std::string
std::vector
std::cout
```

工程头文件中不建议：

```cpp
using namespace std;
```

---

# CMake

## 17. CMakeLists.txt

定义构建目标：

```cmake
add_executable(minijudge
    src/main.cpp
    src/Compiler.cpp
    src/Runner.cpp
    src/Checker.cpp
    src/TestCasesFinder.cpp
)
```

头文件目录：

```cmake
target_include_directories(
    minijudge PRIVATE include
)
```

---

## 18. 构建命令

配置：

```bash
cmake -S . -B build
```

编译：

```bash
cmake --build build
```

重要：

```text
修改源码后必须重新 cmake --build build
```

否则：

```text
./build/minijudge
```

仍可能运行旧版本。

---

# 命令行参数

## 19. `argc` 与 `argv`

```cpp
int main(int argc, char* argv[])
```

`argv` 中的命令行参数本质上都是字符串。

运行：

```bash
./build/minijudge examples/ac.cpp
```

对应：

```text
argv[0] = ./build/minijudge
argv[1] = examples/ac.cpp
```

位置参数不能再简单假设固定在 `argv[1]`，使用 `getopt_long()` 后应通过 `optind` 获取。

---

## 19.1 `getopt_long()`

需要：

```cpp
#include <getopt.h>
```

作用：

```text
每次解析一个命令行 option
没有更多 option 时返回 -1
```

基本模型：

```cpp
while ((opt = getopt_long(
    argc,
    argv,
    shortOptions,
    longOptions,
    nullptr
)) != -1) {
    switch (opt) {
    }
}
```

---

## 19.2 短选项

例如：

```cpp
const char* shortOptions = "t:m:h";
```

含义：

```text
t:  -t 必须携带参数
m:  -m 必须携带参数
h   -h 不携带参数
```

示例：

```bash
-t 2000
-h
```

短选项只有一个字符，通常只给常用选项设置。

---

## 19.3 长选项

```cpp
static option longOptions[] = {
    {"time-limit", required_argument, nullptr, 't'},
    {"memory-limit", required_argument, nullptr, 'm'},
    {"help", no_argument, nullptr, 'h'},
    {nullptr, 0, nullptr, 0}
};
```

含义：

```text
required_argument  必须携带参数
no_argument        不携带参数
```

最后：

```cpp
{nullptr, 0, nullptr, 0}
```

是数组结束标记。

有对应短选项时：

```text
-t
--time-limit
```

可以统一返回：

```cpp
't'
```

只有长选项时，也可以自己指定整数常量：

```cpp
enum {
    OPT_FOO = 1000
};
```

然后：

```cpp
{"foo", no_argument, nullptr, OPT_FOO}
```

处理：

```cpp
case OPT_FOO:
```

`switch case` 不要求是字符，只要求是编译期整数常量。

---

## 19.4 `optarg`

`optarg` 由 `<getopt.h>` 声明。

解析：

```bash
-t 2000
```

时：

```text
opt == 't'
optarg -> "2000"
```

可转为：

```cpp
std::string timeText = optarg;
```

---

## 19.5 `optind`

`optind` 指向解析完成后第一个剩余位置参数。

GNU `getopt_long()` 默认允许：

```bash
./minijudge -t 2000 a.cpp
./minijudge a.cpp -t 2000
```

并可能重排 `argv`，把 option 与位置参数整理开。

重排不是随机的，同类参数的相对顺序会保留。

解析结束后：

```text
argv[optind]
```

是第一个位置参数。

当前 MiniJudge 只允许一个源码路径，因此检查：

```cpp
if (argc - optind != 1) {
    // 参数数量错误
}
```

---

## 19.6 `std::from_chars()`

需要：

```cpp
#include <charconv>
```

作用：

```text
字符串 → 数字
```

不抛异常，通过返回结果报告解析状态。

示例：

```cpp
auto result = std::from_chars(
    text.data(),
    text.data() + text.size(),
    value
);
```

检查：

```cpp
result.ec
```

判断解析是否失败。

检查：

```cpp
result.ptr == text.data() + text.size()
```

确认整个字符串都被消费，避免：

```text
2000abc
```

只解析出前面的 `2000`。

当前时间限制还需额外满足：

```cpp
timeLimitMs > 0
```

---

## 19.7 `getopt_long()` 默认错误输出

GNU `getopt_long()` 默认会自行输出：

```text
未知选项
缺少 option 参数
```

因此当前：

```cpp
default:
    return 1;
```

即可，避免重复输出错误。

---

## 19.8 当前 MiniJudge CLI

格式：

```text
minijudge [options] <source_path>
```

默认：

```text
time limit = 1000 ms
memory limit = 64 MiB
```

支持：

```bash
-t 2000
--time-limit 2000
-m 64
--memory-limit 64
```

帮助：

```bash
-h
--help
```

`-h / --help`：

```text
打印帮助
立即退出
返回 0
```

---

# filesystem

## 20. 路径检查

```cpp
std::filesystem::exists(path);
std::filesystem::is_directory(path);
```

区别：

```text
exists()
路径存在

is_directory()
路径是目录
```

---

## 21. 遍历目录

```cpp
for (const auto& entry :
     std::filesystem::directory_iterator(testDir)) {
}
```

目录遍历顺序没有保证。

---

## 22. 文件名操作

```cpp
path.filename()
path.stem()
path.extension()
```

例如：

```text
1.in

filename  1.in
stem      1
extension .in
```

---

## 23. 路径拼接

推荐：

```cpp
std::filesystem::path input =
    std::filesystem::path(testDir) /
    (name + ".in");
```

不要依赖字符串末尾是否有 `/`。

---

# 测试点发现

## 24. 配对规则

同名：

```text
<name>.in
<name>.out
```

组成一个测试点。

例如：

```text
1.in / 1.out
2#1.in / 2#1.out
abc.in / abc.out
```

测试点名称不要求连续。

---

## 25. `set<string>`

分别保存：

```cpp
std::set<std::string> inputNames;
std::set<std::string> outputNames;
```

双向检查 `.in/.out` 是否成对。

注意：

```text
set<string> 按字符串字典序排列

1
10
2
abc
```

---

## 26. 测试名白名单

当前允许：

```text
A-Z
a-z
0-9
_
-
#
.
```

第一版使用 Shell 命令时，白名单比不断补危险字符黑名单更可靠。

---

# Linux 进程

## 27. `fork()`

```cpp
pid_t pid = fork();
```

返回值：

```text
pid < 0  创建失败
pid == 0 当前是子进程
pid > 0  当前是父进程，值为子进程 PID
```

基本模型：

```text
父进程
   │
 fork
 ┌─┴─┐
父   子
```

---

## 28. `pid_t`

专门表示进程 PID 的类型：

```cpp
pid_t pid;
```

系统调用返回 PID 时优先使用 `pid_t`，不要随意写成普通 `int`。

---

## 29. `execv()`

```cpp
execv(program.c_str(), argv);
```

作用：

```text
用新的程序替换当前进程内容
```

重要：

```text
exec 不创建新进程
fork 才创建新进程
```

成功后：

```text
execv() 不会返回
```

返回说明执行失败。

---

## 30. `_exit()`

子进程在 `exec()` 前发生内部错误时：

```cpp
_exit(1);
```

比普通：

```cpp
exit(1);
```

更适合 `fork()` 后的子进程错误路径，可避免重复刷新继承来的 C/C++ 缓冲区等问题。

---

# 文件描述符与重定向

## 31. `open()`

打开输入：

```cpp
open(path, O_RDONLY);
```

打开输出：

```cpp
open(
    path,
    O_WRONLY | O_CREAT | O_TRUNC,
    0644
);
```

含义：

```text
O_WRONLY  只写
O_CREAT   不存在则创建
O_TRUNC   已存在则清空
```

---

## 32. `dup2()`

```cpp
dup2(inputFd, STDIN_FILENO);
```

让：

```text
stdin
```

指向输入文件。

输出：

```cpp
dup2(outputFd, STDOUT_FILENO);
```

等价于 Shell 的：

```bash
< input
> output
```

---

## 33. `close()`

完成 `dup2()` 后：

```cpp
close(inputFd);
close(outputFd);
```

原因：

新的标准输入/输出文件描述符已经建立，原来的 fd 不再需要。

---

# pipe

## 34. `pipe()`

```cpp
int pipeFd[2];
pipe(pipeFd);
```

含义：

```text
pipeFd[0] 读端
pipeFd[1] 写端
```

当前 Runner 用于：

```text
子进程
→ 报告 exec / open / dup2 等 MiniJudge 内部错误
→ 父进程
```

---

## 35. 父子关闭无用端

子进程：

```cpp
close(pipeFd[0]);
```

父进程：

```cpp
close(pipeFd[1]);
```

否则可能导致：

```text
read 一直等不到 EOF
```

---

# waitpid

## 36. 普通 `waitpid()`

```cpp
waitpid(pid, &status, 0);
```

父进程会等待子进程状态变化。

返回后通过 `status` 分析结束原因。

---

## 37. `WNOHANG`

```cpp
waitpid(pid, &status, WNOHANG);
```

非阻塞检查。

返回值：

```text
> 0 子进程已结束并被回收
0   子进程尚未结束
-1  waitpid 出错
```

TLE 需要 `WNOHANG`，否则父进程会一直阻塞，没有机会检查时间。

---

# Zombie

## 38. 僵尸进程

子进程结束后：

```text
程序已经不运行
CPU 不再执行它
```

但内核会暂时保留：

```text
PID
退出状态
终止信号
资源统计等
```

等待父进程：

```cpp
waitpid()
```

回收。

记忆：

```text
kill    让进程终止
waitpid 回收终止后的进程记录
```

---

# 子进程退出状态

## 39. 正常退出

```cpp
if (WIFEXITED(status)) {
    int exitCode = WEXITSTATUS(status);
}
```

例如：

```cpp
return 7;
```

得到：

```text
WIFEXITED == true
WEXITSTATUS == 7
```

---

## 40. 信号终止

```cpp
if (WIFSIGNALED(status)) {
    int sig = WTERMSIG(status);
}
```

例如：

```text
SIGSEGV
SIGTERM
SIGKILL
SIGFPE
```

注意：

信号终止时不能使用：

```cpp
WEXITSTATUS()
```

---

# Signal

## 41. SIGTERM

含义：

```text
Termination Signal
```

默认：

```text
请求进程终止
```

可以被捕获和处理。

---

## 42. SIGSEGV

含义：

```text
Segmentation Violation
```

常见原因：

```text
空指针
野指针
非法内存访问
严重越界
use-after-free
```

默认行为：

```text
终止进程 + core dump
```

即常见：

```text
Segmentation fault
```

---

## 43. SIGKILL

强制终止进程：

```cpp
kill(pid, SIGKILL);
```

特点：

```text
不能捕获
不能忽略
不能处理
```

MiniJudge 超时后优先使用 `cgroup.kill` 终止用户程序及其后代；整组终止失败时，才使用 `SIGKILL` 兜底终止直接子进程。

---

## 44. SIGFPE

常见于：

```text
整数除 0
非法算术操作
```

默认可能：

```text
终止 + core dump
```

MiniJudge 判为 RE。

---

# 运行时间

## 45. `steady_clock`

```cpp
auto start =
    std::chrono::steady_clock::now();
```

适合测量时间间隔。

当前使用：

```cpp
std::chrono::duration_cast<
    std::chrono::microseconds
>(now - start).count();
```

---

## 46. `timeUs`

命名：

```text
Us = microseconds = 微秒
Ms = milliseconds = 毫秒
```

关系：

```text
1 s
= 1000 ms
= 1,000,000 us
```

例如：

```cpp
timeUs / 1000.0
```

转换为毫秒。

---

## 47. Wall Time

当前 MiniJudge 使用的是 wall time：

```text
现实世界从开始到结束经过的时间
```

包括：

```text
真正占用 CPU
等待调度
等待 I/O
虚拟机未获得 CPU 的时间
```

因此会受到：

```text
系统负载
CPU 降频
虚拟机调度
```

影响。

后续可考虑 CPU time 与 wall time 分开限制。

---

# TLE

## 48. TLE 基本流程

```text
waitpid(WNOHANG)
↓
仍在运行
↓
检查 elapsed time
↓
超过限制
↓
尝试 cgroup.kill
├── 成功 → 整组终止
└── 失败 → SIGKILL 兜底
↓
waitpid 回收
↓
TLE
```

时间限制：

```text
默认 1000 ms
可通过 -t / --time-limit 自定义
```

---

## 49. `usleep()`

```cpp
usleep(1000);
```

单位：

```text
微秒
```

即：

```text
1000 us = 1 ms
```

作用：

避免：

```text
waitpid
检查时间
waitpid
检查时间
...
```

形成 busy waiting。

注意：

```text
usleep(1000)
```

不保证精确 1ms 后重新运行。

---

## 50. Busy Waiting

错误形式：

```cpp
while (true) {
    waitpid(pid, &status, WNOHANG);
}
```

会不断占用 CPU。

当前使用短暂：

```cpp
usleep(1000);
```

降低轮询开销。

---

# RE 与 TLE

## 51. Runner 状态

```cpp
enum class RunStatus {
    Ok,
    RuntimeError,
    TimeLimitExceeded,
    MemoryLimitExceeded,
    InternalError
};
```

返回：

```cpp
struct RunResult {
    RunStatus status;
    long long timeUs;
    long long memoryBytes;
};
```

使用 enum class 避免：

```text
0 / 1 / 2 / 3
```

等魔法数字。

---

## 52. InternalError

表示：

```text
MiniJudge 自身运行错误
```

例如：

```text
pipe 失败
fork 失败
waitpid 失败
read 失败
子进程 open / dup2 / exec 失败
```

区别：

```text
用户程序错误 → RE
MiniJudge 自己出错 → InternalError
```

---

## 53. TLE 不等于 SIGKILL

错误结论：

```text
WTERMSIG == SIGKILL
→ TLE
```

因为用户程序也可能：

```cpp
kill(getpid(), SIGKILL);
```

这种情况应该是：

```text
RE
```

正确概念（已排除 cgroup OOM kill）：

```text
MiniJudge 因超时主动调用 cgroup.kill
+
整组终止失败时才发送 SIGKILL
+
最终确认子进程因此终止
→ TLE
```

---

# Core Dump

## 54. Core Dump

程序发生严重异常时，Linux 可以保存进程崩溃时的信息，用于调试。

常见触发：

```text
SIGSEGV
SIGFPE
```

Ubuntu 可能由：

```text
Apport
```

处理 core dump。

---

## 55. `core_pattern`

查看：

```bash
cat /proc/sys/kernel/core_pattern
```

当前环境类似：

```text
|/usr/share/apport/apport ...
```

开头：

```text
|
```

表示：

```text
core dump 被管道交给用户态程序处理
```

---

## 56. `ulimit -c`

查看：

```bash
ulimit -c
```

虽然可能显示：

```text
0
```

但在 `core_pattern` 使用管道处理程序时，不能简单认为 Apport 就不会被触发。

---

## 57. Core Dump 导致 RE/TLE 误判

实际现象：

```text
用户程序几毫秒时发生 SIGSEGV
↓
Apport 开始处理 core dump
↓
waitpid(WNOHANG) 长时间返回 0
↓
wall time 超过限制
↓
MiniJudge 误以为仍在正常运行
↓
错误判 TLE
```

---

## 58. `/proc/<pid>/status`

Linux 可以通过：

```text
/proc/<pid>/status
```

查看进程状态。

例如：

```text
/proc/1234/status
```

其中：

```text
CoreDumping: 1
```

表示进程正在进行 core dump。

---

## 59. `isCoreDumping()`

当前思路：

```text
打开 /proc/<pid>/status
↓
逐行 getline
↓
寻找 CoreDumping:
↓
读取字段值
↓
1 → 正在 core dump
0 → 未在 core dump
```

读取文件：

```cpp
std::ifstream file(path);

while (std::getline(file, line)) {
}
```

注意：

```cpp
getline(file, line)
```

表示：

```text
从文件读取一行到 line
```

不是“在文件中寻找 line”。

---

## 60. CoreDumping 的竞态条件

以下操作不是原子的：

```text
waitpid(WNOHANG)
↓
读取 /proc/<pid>/status
↓
kill(SIGKILL)
```

中间进程状态可能发生变化。

因此：

```text
一次 CoreDumping 状态
不能作为最终 verdict 的唯一依据
```

最终仍需检查：

```cpp
WIFSIGNALED(status)
WTERMSIG(status)
```

---

# 测试

## 61. 稳定 TLE 测试

不要使用：

```text
“大概需要 1 秒”的计算程序
```

作为 TLE 回归测试。

稳定测试：

```cpp
int main() {
    while (true) {}
}
```

结果必须：

```text
TLE
```

---

## 62. RE 回归测试

至少覆盖：

```text
非零 return
SIGTERM
SIGKILL
SIGSEGV
SIGFPE / divide by zero
```

预期均为：

```text
RE
```

其中：

```text
SIGSEGV / SIGFPE
```

可能因为 core dump 明显变慢。

---

## 63. 当前完整回归集合

至少检查：

```text
AC
WA
CE
RE
TLE
MLE
```

修改 Runner 后不能只测试新加入的功能。

---

# Git

## 64. 基本流程

```text
工作区
↓ git add
暂存区
↓ git commit
本地仓库
↓ git push
远程仓库
```

---

## 65. Commit Message

格式：

```text
type: description
```

例如：

```bash
git commit -m "feat: implement TLE detection"
```

修复：

```bash
git commit -m \
"fix: distinguish RE from TLE during core dumps"
```

---

## 66. 修改最近提交

```bash
git add .
git commit --amend --no-edit
```

`amend` 会重新创建最近一次 commit，因此：

```text
commit hash 会改变
```

如果该提交已经 push 到远程，再 amend 后普通 `git push` 会因为历史分叉被拒绝。

确认需要用本地 amend 后的提交替换远程时：

```bash
git push --force-with-lease
```

优先使用：

```text
--force-with-lease
```

而不是普通 `--force`。

---

## 67. 撤回提交但保留修改

```bash
git reset --soft HEAD~1
```

不要随意：

```bash
git reset --hard
```

---

## 67.1 Git 分支

`main` 通常保存稳定版本。

单人、小步开发时不必为了“工程化”强行多分支。

适合新建 feature 分支的情况：

```text
改动较大
需要多个 commit
中途可能破坏稳定版本
方案可能最终被放弃
```

例如：

```bash
git switch -c feature/compiler-process
```

功能完成并验证后再合并回 `main`。

---

# Linux 文档

## 68. `man` 章节

常见章节：

```text
1  可执行命令
2  系统调用
3  库函数
5  文件格式
7  杂项 / 协议 / 约定
```

例如：

```bash
man 2 fork
man 3 execv
```

文档中：

```text
fork(2)
printf(3)
```

括号中的数字也是 man page 章节号。

---

# Linux 开发环境

## 69. `ENOSPC`

错误：

```text
ENOSPC: no space left on device
```

表示：

```text
文件系统空间不足
```

检查：

```bash
df -h
```

查目录占用：

```bash
du -h --max-depth=1 ~ | sort -h
```

常见缓存目录：

```text
~/.cache
~/.vscode-server
```

---

# 工程设计结论

## 70. 不要过早抽象

只有状态需要：

```text
保存
传递
统一判断
```

时才引入额外类型。

Runner 最初只有成功/失败时不需要 enum。

加入：

```text
OK
RE
TLE
InternalError
```

后：

```cpp
enum class RunStatus
```

开始具有实际价值。

---

## 71. 最终状态比中间动作更重要

错误思路：

```text
我调用了 kill(SIGKILL)
→ 一定 TLE
```

正确思路：

```text
我为什么发送 signal
+
waitpid 最终观察到什么
→ 决定 verdict
```

系统编程中：

```text
状态可能在两个系统调用之间变化
```

不能只依赖一次状态快照。

---

## cgroup v2：内存限制与进程组清理

### 1. cgroup 的作用

cgroup 是 Linux 内核提供的资源管理机制，可以把一组进程放进同一个控制组，并统一进行资源统计和限制。

MiniJudge 当前使用：

```text
memory.max
memory.swap.max
memory.events
memory.peak
cgroup.procs
cgroup.kill
cgroup.events
```

cgroup 控制的是一组进程，而不是单独一个 PID。这里的“进程组”指控制组中的进程集合，不是由 PGID 标识的 POSIX 进程组。

---

### 2. cgroup v2 基本结构

MiniJudge 要求运行前配置好委派给普通用户的 cgroup 子树；一种配置结构如下：

```text
/sys/fs/cgroup/minijudge/
├── manager/
└── run/
```

其中（manager 用于放置管理进程，由环境配置准备；当前代码只创建和删除 run）：

```text
minijudge/
→ 长期存在的 delegated cgroup

run/
→ 每个测试点临时创建
→ 用户程序及其后代进程进入这里
→ 评测结束后删除
```

---

### 3. memory.max

设置 cgroup 的硬内存限制：

```text
memory.max = 67108864
```

表示：

```text
64 MiB = 64 * 1024 * 1024 bytes
```

MiniJudge 内部使用 bytes 保存资源数据，输出时再转换为 MiB。

---

### 4. memory.swap.max

```text
memory.swap.max = 0
```

禁止该 cgroup 使用 swap。

这样测试时内存限制行为更直接，不会因为 swap 延缓 OOM。

---

### 5. 子进程加入 cgroup

child 在 `execv()` 前将自己的 PID 写入：

```text
cgroup.procs
```

逻辑：

```text
fork
↓
child
↓
getpid()
↓
写入 cgroup.procs
↓
execv
```

由 child 自己加入 cgroup，可以避免：

```text
parent fork
↓
child 已经开始执行
↓
parent 才来得及移动 child
```

这种竞态问题。

---

### 6. MLE 判定

只看：

```text
SIGKILL
```

不能判断 MLE。

因为：

```text
MiniJudge 超时主动 SIGKILL
→ TLE

cgroup OOM killer SIGKILL
→ MLE
```

MiniJudge 使用：

```text
memory.events
```

中的：

```text
oom_kill
```

进行判断。

例如：

```text
oom_kill 1
```

说明 cgroup 中发生过 OOM kill。

排除 MiniJudge 内部错误后，当前判定优先级：

```text
oom_kill > 0
→ MLE

否则 timeOut && SIGKILL
→ TLE

其他 signal
→ RE
```

---

### 7. memory.peak

```text
memory.peak
```

记录该 cgroup 曾达到的最大内存使用量，单位为 bytes。

MiniJudge 将其作为测试点峰值内存；当前在清理残留后代进程之前读取，因此不包含读取之后可能出现的新峰值。它是控制组统计口径，不等同于单进程 RSS。

内部：

```text
memoryBytes
```

显示：

```cpp
memoryBytes / 1024.0 / 1024.0
```

转换为 MiB。

相比 `wait4()` 的 `ru_maxrss`，cgroup 的 `memory.peak` 更适合作为 MiniJudge 的统计口径，因为：

```text
资源限制
峰值统计
OOM 判定
```

都来自同一套 cgroup 机制，并且能够覆盖用户程序产生的多个进程。

---

### 8. waitpid 不代表整个用户程序已经结束

测试程序：

```cpp
#include <unistd.h>

int main() {
    pid_t descendantPid = fork();
    if (descendantPid < 0) return 1;

    if (descendantPid == 0) {
        while (true) {
            sleep(1);
        }
    }

    return 0;
}
```

用户程序主进程退出后，MiniJudge 等待它直接创建的评测进程：

```cpp
waitpid(judgeChildPid, &status, 0)
```

只能确认 MiniJudge 直接创建的 child 已结束。

用户程序 fork 出来的后代仍可能继续运行。

此时：

```text
cgroup.procs
```

仍然存在 PID，直接删除 cgroup 会失败：

```text
Device or resource busy
```

结论：

```text
waitpid(直接子进程)
≠
整个用户 workload 已结束
```

---

### 9. cgroup.kill

向：

```text
cgroup.kill
```

写：

```text
1
```

可以终止该 cgroup 中的全部剩余进程。

它是只写控制接口：

```bash
echo 1 > cgroup.kill
```

不能通过：

```bash
cat cgroup.kill
```

读取状态。

---

### 10. cgroup.kill 也存在时序问题

写入：

```text
cgroup.kill = 1
```

只表示向内核发出 kill 请求。

函数返回时，进程不一定已经彻底退出。

如果马上：

```text
killCgroup()
↓
removeCgroup()
```

可能仍然得到：

```text
Device or resource busy
```

所以 `killCgroup()` 在写入 `1` 后继续读取：

```text
cgroup.events
```

等待：

```text
populated 0
```

再返回。当前等待循环没有超时上限。

最终清理流程：

```text
读取 oom_kill
↓
读取 memory.peak
↓
cgroup.kill
↓
等待 populated == 0
↓
删除 cgroup
```

---

### 11. cgroup delegation

普通用户默认不能直接在：

```text
/sys/fs/cgroup
```

下创建 cgroup。

因此需要由管理员把一棵 cgroup 子树委派给普通用户管理。

delegation 后：

```bash
./build/minijudge ...
```

可以直接运行，不需要：

```bash
sudo ./build/minijudge ...
```

管理员权限只应该用于环境配置，不应该让用户评测程序以 root 权限运行。

仓库中的 `scripts/setup-cgroup.sh` 提供了当前环境的最小配置流程：

```bash
./scripts/setup-cgroup.sh
```

脚本会检查 cgroup v2 和 `memory` controller，创建：

```text
/sys/fs/cgroup/minijudge/
└── manager/
```

然后在 `minijudge` 层开启 `+memory`，并把 `minijudge`、`cgroup.procs` 和 `cgroup.subtree_control` 的管理权限交给当前用户。脚本通过 `sudo` 执行需要管理员权限的操作；评测命令本身仍应以普通用户身份运行：

```bash
./build/minijudge examples/ac.cpp
```

当前脚本会把执行它的当前 shell 放入 `manager`。打开新的 shell 后，运行 MiniJudge 前需要重新执行脚本；脚本的重复执行和失败回滚仍需完善。

---

### 12. MB 与 MiB

```text
1 MB  = 1000 * 1000 bytes
1 MiB = 1024 * 1024 bytes
```

MiniJudge 的：

```text
-m 64
```

当前表示：

```text
64 MiB
```

---

### 常见坑

1. 只用 `SIGKILL` 无法区分 MLE 和 TLE；
2. 当前实现未启用 `RLIMIT_AS`；同时设置它可能先导致 `bad_alloc`，无法可靠得到 cgroup OOM 事件；
3. `waitpid()` 在当前调用方式下只等待指定的直接子进程，不能保证后代进程结束；
4. `cgroup.kill` 返回不代表进程已经立即全部消失；
5. cgroup 还有进程时直接删除会得到 `EBUSY`；
6. `memory.peak` 单位是 bytes；
7. cgroup delegation 和普通文件目录权限不是一回事。

---

## Compiler：从 `system()` 到 `fork + execvp`

原实现：

```cpp
std::system("g++ ...");
```

会经过 Shell。

当前实现：

```text
fork
├── child
│   ├── open compile.log
│   ├── dup2(stderr)
│   └── execvp("g++", argv)
│
└── parent
    └── waitpid
```

### `execv` 与 `execvp`

- `execv()`：需要明确的可执行文件路径。
- `execvp()`：会根据 `PATH` 查找程序，因此可以直接执行 `"g++"`。
- 参数通过 `argv[]` 独立传递，不经过 Shell 拆词。

### 常见坑

`open()` 和 `dup2()` 应分别检查失败，否则 `open()` 失败可能最终只看到 `dup2: Bad file descriptor`。

---

## 管道 EOF 与 `O_CLOEXEC`

Runner 使用管道区分：

- 用户程序自身的 RE；
- MiniJudge 在 `execv()` 前的内部错误。

```text
read() > 0
→ 收到子进程写入的错误标记

read() == 0
→ EOF：所有写端均已关闭，且没有错误标记

read() == -1
→ read() 自身失败
```

`O_CLOEXEC` 会在 `execv()` 成功时自动关闭子进程继承的管道写端。

如果没有它，用户程序及其后代可能继续持有写端，父进程无法收到 EOF。

---

## `cgroup.kill` 与 `waitpid`

两者职责不同：

```text
cgroup.kill
→ 终止用户程序及其后代进程

waitpid
→ 回收 MiniJudge 的直接子进程退出状态
```

用户程序 `fork()` 的子进程默认继承父进程所在的 cgroup，因此：

```text
run/
├── user_program
├── child 1
├── child 2
└── ...
```

可以通过 `cgroup.kill` 整组终止。

TLE 当前处理：

```text
超时
↓
cgroup.kill
├── 成功 → 整组终止
└── 失败 → kill(pid, SIGKILL) 兜底终止直接子进程
↓
waitpid
↓
回收直接子进程
```

### 结论

- `kill` / `cgroup.kill` 负责让进程停止运行。
- `waitpid` 负责回收直接子进程。
- `cgroup.kill` 能避免用户通过 `fork()` 留下后台进程。

---

## cgroup v2：进程数限制

`pids.max` 限制一个 cgroup 中允许存在的进程数量。

```text
pids.max      → 进程数上限
pids.current  → 当前进程数
pids.events   → 进程数限制触发事件
```

用户程序 `fork()` 出来的子进程默认继承父进程所在的 cgroup，因此 fork bomb 无法通过创建后代绕过限制。

本项目当前：

```text
pids.max = 64
```

实测 fork bomb 达到上限后：

```text
pids.current = 64
pids.events: max > 0
```

其中 `pids.events` 的 `max` 是触发进程数上限的累计事件次数，不是上限值；上限看 `pids.max`，当前数量看 `pids.current`。后续 `fork()` 会被内核拒绝，评测结束后通过 `cgroup.kill` 清理整组进程并删除临时 cgroup。当前不会因为触发 `pids.max` 单独产生新的 verdict；程序若继续运行会按 TLE 处理，若自行退出则仍按最终退出状态判定。

---

# MiniJudge 当前状态

## 已完成

* CMake 构建、源码编译及 CE 判定
* 自动发现测试点并校验同名 `.in` / `.out`
* AC / WA / CE / RE / TLE / MLE 判定
* Compiler 使用 `fork + execvp + dup2 + waitpid` 调用 `g++`
* Runner 使用 `fork + execv + dup2 + waitpid(WNOHANG)`
* pipe 报告子进程准备阶段的内部错误
* Core Dump 期间 RE/TLE 误判处理
* `-t / --time-limit`，默认 1000 ms
* `-m / --memory-limit`，默认 64 MiB
* cgroup v2 内存限制、禁用 swap 和进程数限制
* `memory.events` OOM 检测、`memory.peak` 峰值内存统计
* 内置 Checker：忽略空行和行末空白，其余内容逐字符比较
* 在手工委派的 cgroup 子树中运行
* `cgroup.kill` 后代进程清理，等待 `populated 0` 后删除控制组
* 使用主进程 PID 隔离每个实例的 cgroup 和 `tmp/run-<PID>/` 工作目录，支持并发运行

以上按当前源码整理，不代表本次文档更新重新运行了完整回归测试。

当前评测流程：

```text
CLI：source_path / time limit / memory limit
↓
发现并校验测试点 → Compiler → user_program
↓
Runner（逐测试点）
├─ 创建 `run-<PID>` cgroup，配置 memory.max / memory.swap.max / pids.max
├─ fork → 子进程加入 cgroup → dup2 → execv
├─ waitpid(WNOHANG)，检查超时和 CoreDumping
├─ 超时时 cgroup.kill，失败后 SIGKILL 兜底
├─ 读取 oom_kill / memory.peak
├─ cgroup.kill → 等待 populated 0 → 删除 cgroup
└─ 返回状态、timeUs、memoryBytes
↓
正常退出且退出码为 0 → Checker → AC / WA
否则 → MLE / TLE / RE / Run failed
```

# 当前限制

* cgroup delegation 仍需要先执行 `scripts/setup-cgroup.sh`，脚本的重复执行和失败回滚仍需完善
* 尚未实现完整 sandbox 和其他系统资源限制
* 使用 wall time，包含 Runner 的创建、等待和清理开销，受机器负载和虚拟机调度影响
* 尚未限制 CPU time；core dump 可能导致 RE 返回较慢
* `killCgroup()` 等待 `populated 0` 暂无超时上限，异常路径清理仍需完善
* OOM 事件和峰值内存在清理前读取，后代进程尚未全部停止时统计仍可能变化
* Compiler 仍依赖外部 `g++` 命令
* `Ctrl+C` 等外部信号中断时，正常 cleanup 可能来不及执行，临时 cgroup 和工作目录可能残留
* 必须从项目根目录运行

---

# 当前唯一下一步

```text
完成当前版本收尾，
判断 MiniJudge 是否达到阶段性可投版本，
再切换到 Linux Socket / 网络编程。
```

暂缓：

```text
线程池
并行评测
Docker 沙箱
Web 页面
数据库
分布式评测
复杂设计模式
```
