# MiniJudge

A lightweight local C++ judge for Linux.

MiniJudge 是一个轻量级本地 C++ 评测工具：编译源码、自动发现测试点，在独立 cgroup 中执行每个测试点，输出 **AC / WA / CE / RE / TLE / MLE**、CPU 时间和峰值内存。

## Features

- 使用 `g++` 编译 C++17 提交，自动发现并校验 `.in` / `.out` 测试数据。
- 判定 AC / WA / CE / RE / TLE / MLE，WA 展示首处差异及期望、实际输出。
- 通过 `fork` / `exec` / `waitpid` 管理评测进程，使用 `dup2` 重定向输入输出。
- 基于 cgroup v2 限制内存和进程数量，统计测试程序及其后代的累计 CPU 时间与峰值内存。
- CPU 时间限制配合 wall-clock watchdog（墙钟超时保护），通过 `cgroup.kill` 清理后代进程。
- 固定数量的 worker threads（工作线程）动态领取测试点，支持 `-j` 指定并发数，按测试点名称顺序输出结果。
- 每个实例使用独立临时目录，每个测试点使用独立 cgroup 和输出文件。

## Quick Start

需要 Linux、支持 C++17 的 `g++`、CMake ≥ 3.12，以及启用 memory / pids controller 的 cgroup v2。内核须提供 `memory.swap.max`、`memory.peak`、`cgroup.kill` 等接口。

在 Linux / WSL2 的 Bash 中执行：

```bash
git clone https://github.com/qqbb111/MiniJudge.git
cd MiniJudge
cmake -S . -B build
cmake --build build

./scripts/setup-cgroup.sh
./build/minijudge examples/ac.cpp
```

配置脚本在需要的位置调用 `sudo`，创建 `/sys/fs/cgroup/minijudge/` 子树、启用 controller，并将调用它的 shell 移入 `manager`。**每个新的 shell 会话都需要先运行脚本，再在同一 shell 中以普通用户运行 MiniJudge。**

## Usage

```bash
./build/minijudge [options] <source_path>
./build/minijudge -t 1000 -m 64 -j 4 examples/ac.cpp
./build/minijudge --jobs 1 examples/wa.cpp
./build/minijudge --help
```

| 参数 | 含义 | 默认值 |
| --- | --- | --- |
| `-t, --time-limit <ms>` | 每个测试点的累计 CPU 时间限制 | `1000 ms` |
| `-m, --memory-limit <MiB>` | 每个测试点的内存限制 | `64 MiB` |
| `-j, --jobs <N>` | 并行 worker 数量 | 硬件并发度；检测失败时为 4 |
| `-h, --help` | 显示帮助 | — |

`-t`、`-m`、`-j` 必须为正整数。worker 数量不会超过测试点数量。程序通过相对路径读取 `tests/`、写入 `tmp/`，须从项目根目录启动。

### Test Data

同名 `.in` 和 `.out` 文件组成一个测试点。仓库自带的测试数据是两整数求和，可直接用于 `examples/ac.cpp` 等示例。

```text
tests/
├── 1.in
├── 1.out
├── 2#1.in
└── 2#1.out
```

名称允许 `A-Z a-z 0-9 _ - # .`，不要求连续数字。缺少配对文件会停止评测；其他扩展名被忽略。Checker 忽略空行和行末空格、Tab、`\r`，保留行首及行内空白。

### Output

以下展示格式，时间和内存是示意值，各行可来自不同提交：

```text
Test 1: AC (1.834 ms, 0.535 MiB)
Test 2: WA (2.431 ms, 0.535 MiB)
  First difference at line 1
  Expect: 30
  Actual: 31
Test 300: TLE (1003.214 ms, 1.203 MiB)
```

WA 的行号从 1 开始，指**去掉空行后的比较序列**；某侧提前结束时显示 `<EOF>`。`Expect:` 和 `Actual:` 标签等宽，便于对照。

测试行中的时间是 cgroup 累计 CPU 时间，内存是 cgroup 峰值。stderr 另输出 `Judge elapsed: ... ms`，表示整个并行评测阶段的墙钟耗时，不包含编译。

编译失败输出 `<source_path> CE`；评测器内部错误显示 `Run Failed`，读取答案失败显示 `Judge Failed`。临时文件位于 `tmp/run-<pid>/`，正常评测结束后清理。

## Architecture

```text
source.cpp
    |
    v
Compiler (g++)
    |
    v
validated test cases --> worker threads
                            |
                            +--> Runner --> per-test cgroup --> fork/exec
                            |                                      |
                            |                                      +--> stdin  <- test.in
                            |                                      +--> stdout -> actual.out
                            |
                            +--> Checker --> AC / WA + first difference
```

`main.cpp` 先发现并校验测试数据，再编译一次源码，交给 workers 执行。Runner 监控进程、读取资源统计并清理后代；运行成功后才由 Checker 比较答案。

| 模块 | 职责 |
| --- | --- |
| `src/Compiler.cpp` | 编译源码，保存编译错误日志 |
| `src/TestCasesFinder.cpp` | 发现并校验测试数据 |
| `src/Runner.cpp` | 启动、监控和回收评测进程 |
| `src/Cgroup.cpp` | 配置资源限制、读取统计、清理控制组 |
| `src/Checker.cpp` | 文本比较和 WA 差异定位 |
| `src/main.cpp` | 参数解析、并行调度、结果输出 |

## Resource Control

每个测试点对应 `/sys/fs/cgroup/minijudge/run-<pid>-<test_name>/`：

| cgroup v2 接口 | 用途 |
| --- | --- |
| `memory.max` | 限制内存 |
| `memory.swap.max = 0` | 禁用该组进程的 swap |
| `pids.max = 64` | 限制任务数量，约束提交创建的进程 / 线程 |
| `cpu.stat` / `usage_usec` | 累计 CPU 时间统计与超时检测 |
| `memory.events` / `oom_kill` | 检测 OOM kill，判定 MLE |
| `memory.peak` | 读取峰值内存 |
| `cgroup.kill` | 终止组内残留后代进程 |

wall-clock watchdog 的阈值为 CPU 时间限制的 3 倍，用于终止 `sleep`、阻塞等低 CPU 占用程序。当前检测到 core dump 时会跳过超时终止，可能使 RE 返回变慢。

MiniJudge 提供资源控制；当前未实现 syscall filtering（系统调用过滤）、namespaces、文件系统或网络隔离，不能作为运行不可信代码的完整安全沙箱。

## Parallel Judging

固定数量的工作线程通过共享索引动态领取任务。互斥锁只保护领取过程，实际评测在锁外执行；所有线程结束后，主线程统一按名称顺序打印结果。

使用同一组数据比较不同并发数：

```bash
bash scripts/benchmark.sh
```

脚本对 `examples/ac_cpubound.cpp` 分别使用 1 / 2 / 4 / 8 / 12 / 16 / 20 个 workers，每档运行 5 次，stderr 显示评测阶段耗时。测试点来自当前 `tests/`；可用下列命令生成额外 50 个固定种子的求和测试点：

```bash
g++ -std=c++17 tests/generate.cpp -o build/generate-tests
./build/generate-tests
```

这些 `random_*.in` / `.out` 已被 Git 忽略。比较性能时应固定测试集、时间限制和机器环境，并检查 verdict；并发越高不保证越快。

## Verification

完成上述构建和当前 shell 的 cgroup 配置后：

```bash
bash scripts/regression.sh
```

现有回归脚本覆盖 AC、WA、CE、RE、core dump、CPU 超时、wall-clock watchdog 和 MLE。

## Current Limitations

- 依赖 Linux 和 cgroup v2；运行前必须配置 delegation（资源子树授权）。
- 仅支持普通文本比较；没有 special judge（特殊判题器）或交互评测。
- 不提供系统调用、文件系统或网络沙箱；未限制输出文件大小和打开文件数。
- 进程 / 线程上限固定为 64；CPU 时间通过轮询采样，短时间限制存在采样误差。
- core dump 等待和 `cgroup.kill` 清理等待目前没有独立超时。
- `Ctrl+C` 或其他异常退出可能遗留临时目录和 cgroup。

## Notes

实现机制与相关 Linux / C++ 知识：

- [cgroup v2：资源限制、统计与后代进程清理](docs/notes/04-cgroup-v2.md)
- [原有完整学习笔记](notes/learning-notes.md)
