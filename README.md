# MiniJudge

MiniJudge 是一个运行在 Linux 环境下的轻量级本地 C++ 代码评测工具。

> 🌾 **100% 古法编程** · 手搓 · 手调 · 手测 ( •̀ ω •́ )✧

支持编译待评测源码、自动发现测试点、批量运行程序、输入输出重定向、运行时间统计以及 AC、WA、CE、RE、TLE 判定。

## 当前功能

* 通过命令行指定待评测的 C++ 源码
* 支持通过 `-t` / `--time-limit` 自定义时间限制，默认 `1000 ms`
* 支持 `-h` / `--help` 查看命令行帮助
* 使用 `g++` 编译源码
* 将编译错误保存到 `tmp/compile.log`
* 自动扫描并校验 `tests/` 目录中的测试数据
* 支持字符串测试点名称
* 使用 `fork()` 创建独立评测进程
* 使用 `execv()` 执行用户程序
* 使用 `dup2()` 完成标准输入、标准输出重定向
* 使用 `waitpid()` 获取用户程序退出状态
* 使用 `WNOHANG` 非阻塞轮询进程状态
* 捕获非零退出码及信号终止，判定 RE
* 检测运行超时并通过 `SIGKILL` 终止超时进程
* 处理 core dump 导致的 RE/TLE 误判问题
* 统计每个测试点运行时间
* 使用 `diff -wB` 比较实际输出与标准答案
* 使用 CMake 管理项目构建

当前支持以下评测结果：

* `AC`：答案正确
* `WA`：答案错误
* `CE`：编译错误
* `RE`：运行时错误
* `TLE`：超过时间限制
* `Run failed`：MiniJudge 内部运行错误

## 项目结构

```text
MiniJudge/
├── CMakeLists.txt
├── README.md
├── .gitignore
│
├── include/
│   ├── Compiler.h
│   ├── Runner.h
│   ├── Checker.h
│   └── TestCasesFinder.h
│
├── src/
│   ├── main.cpp
│   ├── Compiler.cpp
│   ├── Runner.cpp
│   ├── Checker.cpp
│   └── TestCasesFinder.cpp
│
├── examples/
├── tests/
├── tmp/
└── notes/
```

模块职责：

* `Compiler`：编译待评测源码
* `Runner`：创建评测进程、重定向输入输出、统计运行时间并判断运行状态
* `Checker`：比较实际输出与标准答案
* `TestCasesFinder`：发现并校验测试数据
* `main.cpp`：解析命令行参数并组织完整评测流程

`build/` 和 `tmp/` 中生成的临时文件不会提交到 Git 仓库。

## 环境要求

* Linux
* g++，支持 C++17
* CMake 3.10 或更高版本
* GNU `diff`

## 获取项目

```bash
git clone https://github.com/qqbb111/MiniJudge.git
cd MiniJudge
```

## 构建项目

在项目根目录执行：

```bash
cmake -S . -B build
cmake --build build
```

构建完成后，可执行文件位于：

```text
build/minijudge
```

修改 MiniJudge 源码后，需要重新执行：

```bash
cmake --build build
```

否则运行的仍可能是旧版本可执行文件。

## 运行项目

命令格式：

```bash
./build/minijudge [options] <source_path>
```

使用默认 `1000 ms` 时间限制：

```bash
./build/minijudge examples/ac.cpp
```

自定义时间限制：

```bash
./build/minijudge -t 2000 examples/tle.cpp
```

等价写法：

```bash
./build/minijudge --time-limit 2000 examples/tle.cpp
```

在 GNU `getopt_long()` 默认解析方式下，选项也可以放在源码路径之后：

```bash
./build/minijudge examples/tle.cpp --time-limit 2000
```

查看帮助：

```bash
./build/minijudge -h
./build/minijudge --help
```

帮助信息：

```text
Usage: ./build/minijudge [options] <source_path>

Options:
  -t, --time-limit <ms>  Set time limit in milliseconds (default: 1000)
  -h, --help             Show this help message
```

时间限制必须为正整数。非法参数、缺少源码路径或提供多个源码路径时，程序会输出错误并退出。

当前版本使用相对路径访问 `tests/` 和 `tmp/`，因此需要从项目根目录启动。

## 测试数据格式

测试数据存放在 `tests/` 目录。

同名 `.in` 和 `.out` 文件组成一个测试点：

```text
tests/
├── 1.in
├── 1.out
├── 2#1.in
├── 2#1.out
├── sample.in
└── sample.out
```

测试点名称不要求为连续数字。

当前允许：

```text
A-Z  a-z  0-9  _  -  #  .
```

如果 `.in` 与 `.out` 未成对出现，程序会输出错误并停止评测。

其他扩展名文件会被忽略。

## 评测流程

```text
解析命令行参数
    │
    ▼
读取源码路径和时间限制
    │
    ▼
发现并校验测试点
    │
    ▼
编译待评测源码
    │
    ├── 编译失败 ──► CE
    │
    ▼
fork 创建评测子进程
    │
    ├── dup2 重定向 stdin / stdout
    │
    └── execv 执行用户程序
    │
    ▼
父进程 waitpid(WNOHANG)
    │
    ├── 超时 ──────► SIGKILL ──► TLE
    ├── 信号终止 ──► RE
    ├── 非零退出码 ► RE
    │
    ▼
比较实际输出
    │
    ├── 输出相同 ──► AC
    └── 输出不同 ──► WA
```

源码只编译一次，编译成功后依次运行全部测试点。

## 运行状态

`Runner` 使用：

```cpp
enum class RunStatus {
    Ok,
    RuntimeError,
    TimeLimitExceeded,
    InternalError
};
```

并返回：

```cpp
struct RunResult {
    RunStatus status;
    long long timeUs;
};
```

`timeUs` 表示从启动评测到用户程序结束所经过的时间，单位为微秒。

## RE 判定

以下情况会被判定为 RE：

* 用户程序正常退出，但退出码非 `0`
* 用户程序被异常信号终止
* 用户程序主动向自身发送 `SIGKILL`
* `SIGSEGV`、`SIGFPE` 等运行时异常

MiniJudge 使用：

```text
WIFEXITED
WEXITSTATUS

WIFSIGNALED
WTERMSIG
```

分析子进程退出状态。

## TLE 判定

时间限制默认：

```text
1000 ms
```

可通过：

```bash
-t <ms>
--time-limit <ms>
```

自定义。

父进程通过：

```cpp
waitpid(pid, &status, WNOHANG);
```

非阻塞检查用户程序状态，并通过 `steady_clock` 统计 wall time。

用户程序超过时间限制后，MiniJudge 会发送：

```text
SIGKILL
```

并再次调用 `waitpid()` 回收子进程。

TLE 不等价于“程序最终死于 `SIGKILL`”。

只有 MiniJudge 因超过时间限制主动发送 `SIGKILL`，并且最终确认用户进程因此终止时，才判定为 TLE。

用户程序主动触发 `SIGKILL` 仍判定为 RE。

## Core Dump 处理

Ubuntu 等 Linux 环境可能通过 Apport 等程序处理 core dump。

例如：

```text
SIGSEGV
    ↓
Core Dump
    ↓
Apport 处理崩溃信息
    ↓
waitpid 暂时无法完成回收
```

这可能导致已经发生 RE 的程序超过 wall-time 限制，从而被误判为 TLE。

MiniJudge 会读取：

```text
/proc/<pid>/status
```

中的：

```text
CoreDumping:
```

字段，在进程正在进行 core dump 时暂缓 TLE 终止，并最终根据 `waitpid()` 返回的真实终止信号判断 RE/TLE。

## 输出示例

```text
Test 1: AC (7.928 ms)
Test 2: WA (14.350 ms)
Test 2#1: RE (11.318 ms)
Test 300: TLE (2003.545 ms)
```

编译失败：

```text
examples/compile_error.cpp CE
```

MiniJudge 内部运行错误：

```text
Test 1: Run failed (3.214 ms)
```

## 临时文件

编译日志：

```text
tmp/compile.log
```

实际输出：

```text
tmp/actual_<test_name>.out
```

编译后的用户程序：

```text
tmp/user_program
```

## 当前限制

* 必须从项目根目录运行
* 当前运行时间为 wall time，会受到系统负载、调度和虚拟机环境影响
* core dump 处理可能导致 RE 返回明显变慢
* 编译阶段仍通过外部 `g++` 命令完成
* 输出比较仍依赖 GNU `diff`
* 不支持包含任意 Shell 特殊字符的源码路径
* 尚未实现 CPU Time 限制
* 尚未实现内存限制
* 尚未实现其他系统资源限制
* 测试点按照字符串字典序运行

## 后续计划

* 完善 Runner 系统调用错误处理
* 区分 wall time 与 CPU time
* 增加 CPU、内存等资源限制
* 减少对 Shell 命令的依赖
* 完善测试集与项目文档
