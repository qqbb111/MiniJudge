# MiniJudge

MiniJudge 是一个运行在 Linux 环境下的轻量级本地 C++ 代码评测工具。

支持编译待评测源码、自动发现测试点、批量运行程序、重定向输入输出、比较评测结果，并统计每个测试点的运行时间。

## 当前功能

- 通过命令行指定待评测的 C++ 源码
- 使用 `g++` 编译源码，并将编译错误保存到 `tmp/compile.log`
- 自动扫描并校验 `tests/` 目录中的测试数据
- 批量运行多个测试点
- 使用 `fork()` 创建评测子进程
- 使用 `open()` + `dup2()` 重定向标准输入和标准输出
- 使用 `execv()` 执行待评测程序
- 使用 `waitpid()` 获取子进程结束状态
- 使用 `pipe()` 区分评测器启动失败与用户程序运行错误
- 判断用户程序非零退出和信号终止导致的 `RE`
- 使用 `std::chrono::steady_clock` 统计每个测试点的运行时间
- 使用 `diff -wB` 比较实际输出与标准答案
- 输出以下结果：
  - `AC`：答案正确
  - `WA`：答案错误
  - `CE`：编译错误
  - `RE`：运行时错误
  - `Run failed`：MiniJudge 自身未能正常完成运行流程
- 使用 CMake 管理项目构建

当前 `Runner` 已不再依赖 `std::system()`；`Compiler` 和 `Checker` 仍使用 `std::system()` 调用 `g++` 和 `diff`。

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

- `Compiler`：编译待评测源码
- `Runner`：创建评测进程、重定向输入输出、执行程序、判断运行状态并统计时间
- `Checker`：比较实际输出与标准答案
- `TestCasesFinder`：发现并校验测试数据
- `main.cpp`：组织完整评测流程

`build/` 和 `tmp/` 中生成的临时文件不会提交到 Git 仓库。

## 环境要求

- Linux
- g++，支持 C++17
- CMake 3.10 或更高版本
- GNU `diff`

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

## 运行项目

在项目根目录执行：

```bash
./build/minijudge <source_path>
```

例如：

```bash
./build/minijudge examples/accepted.cpp
```

未提供源码路径时：

```text
Usage: ./build/minijudge <source_path>
```

当前版本使用相对路径访问 `tests/` 和 `tmp/`，因此需要从项目根目录启动。

## 测试数据格式

测试数据存放在 `tests/` 目录。

同名的 `.in` 和 `.out` 文件组成一个测试点：

```text
tests/
├── 1.in
├── 1.out
├── 24#2.in
└── 24#2.out
```

测试点名称不要求连续。

当前允许：

```text
A-Z  a-z  0-9  _  -  #  .
```

如果 `.in` 和 `.out` 未成对出现，程序会输出错误并停止评测。其他扩展名文件会被忽略。

## 评测流程

```text
读取源码路径
    │
    ▼
发现并校验测试点
    │
    ▼
编译待评测源码
    │
    ├── 编译失败 ───────► CE
    │
    ▼
fork 创建评测子进程
    │
    ├── open / dup2 / execv 失败 ─► Run failed
    │
    ▼
运行用户程序并统计时间
    │
    ├── 信号终止 ───────► RE
    ├── 非零退出 ───────► RE
    │
    ▼
比较实际输出与标准答案
    │
    ├── 相同 ───────────► AC
    └── 不同 ───────────► WA
```

源码只编译一次，编译成功后依次运行全部测试点。

## 输出示例

```text
Test 1: AC (15.753 ms)
Test 2: WA (21.656 ms)
Test 2#1: RE (4.917 ms)
```

MiniJudge 自身无法正常启动用户程序时：

```text
Test 1: Run failed (0.532 ms)
```

编译失败时：

```text
examples/compile_error.cpp CE
```

编译日志：

```text
tmp/compile.log
```

实际输出：

```text
tmp/actual_<test_name>.out
```

运行时间内部以微秒保存，输出时转换为毫秒并保留三位小数。

## 当前限制

- 必须从项目根目录运行
- 尚未实现 TLE 和主动终止超时进程
- `Compiler` 和 `Checker` 仍依赖 `std::system()`，相关路径暂不支持空格或 Shell 特殊字符
- 尚未实现内存和其他资源限制
- 测试点按照字符串字典序运行
- 当前统计的是墙上时间，包含进程创建、重定向和调度等开销
- Ubuntu Apport 等崩溃收集机制可能导致部分信号型 RE 的显示耗时明显偏大

## 后续计划

- 使用非阻塞 `waitpid()` 实现超时检测，并在超时后终止评测进程
- 移除 `Compiler` 和 `Checker` 中剩余的 `std::system()`
- 增加内存和其他资源限制
- 完善评测报告与错误信息
