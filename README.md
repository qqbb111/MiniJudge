# MiniJudge

MiniJudge 是一个运行在 Linux 环境下的轻量级本地 C++ 代码评测工具。

> 🌾 **100% 古法编程** · 手搓 · 手调 · 手测 ( •̀ ω •́ )✧

支持编译待评测源码、自动发现测试点、多测试点并行评测、输入输出重定向、CPU 时间与峰值内存统计，以及 AC、WA、CE、RE、TLE、MLE 判定。

## 当前功能

* 通过命令行指定待评测的 C++ 源码
* 支持通过 `-t` / `--time-limit` 自定义时间限制，默认 `1000 ms`
* 支持通过 `-m` / `--memory-limit` 自定义内存限制，默认 `64 MiB`
* 支持 `-h` / `--help` 查看命令行帮助
* 使用 `fork()` + `execvp()` 调用 `g++` 编译源码
* 使用 `dup2()` 将编译器标准错误重定向到当前实例的 `compile.log`
* 自动扫描并校验 `tests/` 目录中的测试数据
* 支持字符串测试点名称
* 使用 `fork()` 创建独立评测进程
* 使用 `execv()` 执行用户程序
* 使用 `dup2()` 完成标准输入、标准输出重定向
* 使用 `waitpid()` 获取用户程序退出状态
* 使用 `WNOHANG` 非阻塞轮询进程状态
* 捕获非零退出码及信号终止，判定 RE
* 检测运行超时，并通过 `cgroup.kill` 终止用户程序及其后代进程
* `cgroup.kill` 失败时使用 `SIGKILL` 兜底终止直接评测子进程
* 处理 core dump 导致的 RE/TLE 误判问题
* 使用 cgroup v2 的 `memory.max` 限制内存，并通过 `memory.swap.max = 0` 禁用 swap
* 使用 cgroup v2 的 `pids.max` 将单个测试点的进程数限制为 64，防止 fork bomb 无限创建后代进程
* 使用进程 PID 隔离每个 MiniJudge 实例的 cgroup 与临时工作目录，支持多个实例同时运行
* 为每个测试点创建独立 cgroup 和实际输出文件，支持多个测试点同时评测
* 使用固定数量的 Worker 线程并行执行测试点
* Worker 数量根据 `std::thread::hardware_concurrency()` 自动确定，并限制为不超过测试点数量
* 使用共享索引配合 `std::mutex` 动态分配测试任务，先完成的 Worker 自动领取下一个测试点
* 仅在领取任务时持有互斥锁，实际评测过程不持锁，避免评测流程被串行化
* 所有 Worker 完成后统一按测试点顺序输出结果
* 通过 `memory.events` 的 `oom_kill` 判断 MLE
* 通过 `memory.peak` 统计测试点峰值内存
* 使用 `cgroup.kill` 清理残留后代进程，等待 `populated 0` 后删除控制组
* 基于 cgroup v2 统计测试点 CPU 时间与峰值内存
* 使用 `cpu.stat::usage_usec` 实现 CPU Time Limit，统计测试程序及其后代进程的累计 CPU 消耗
* 保留 Wall Time Watchdog，防止 `sleep`、阻塞等低 CPU 占用程序长期挂起
* 内置输出比较器：忽略空行和行末空白，其余内容逐字符比较
* 使用 CMake 管理项目构建，并通过 `Threads::Threads` 声明线程依赖

当前支持以下评测结果：

* `AC`：答案正确
* `WA`：答案错误
* `CE`：编译错误
* `RE`：运行时错误
* `TLE`：超过时间限制
* `MLE`：发生 OOM kill，超过内存限制
* `Run failed`：MiniJudge 内部运行错误
* `Judge failed`：Checker 读取实际输出或标准答案失败

## 项目结构

```text
MiniJudge/
├── CMakeLists.txt
├── README.md
├── .gitignore
│
├── include/
│   ├── Compiler.h
│   ├── Cgroup.h
│   ├── Runner.h
│   ├── Checker.h
│   └── TestCasesFinder.h
│
├── src/
│   ├── main.cpp
│   ├── Compiler.cpp
│   ├── Cgroup.cpp
│   ├── Runner.cpp
│   ├── Checker.cpp
│   └── TestCasesFinder.cpp
│
├── examples/
│   └── ac_cpubound.cpp
├── tests/
│   └── generate.cpp
├── scripts/
│   └── setup-cgroup.sh
├── tmp/
└── notes/
```

模块职责：

* `Compiler`：编译待评测源码
* `Runner`：创建评测进程、重定向输入输出、统计运行时间和内存并判断运行状态
* `Cgroup`：配置内存限制、加入控制组、读取资源统计并清理后代进程
* `Checker`：比较实际输出与标准答案
* `TestCasesFinder`：发现并校验测试数据
* `main.cpp`：解析命令行参数，组织完整评测流程，并通过固定 Worker 与共享索引调度测试点并行执行

`build/` 和 `tmp/` 中生成的临时文件不会提交到 Git 仓库。

## 环境要求

* Linux
* g++，支持 C++17
* CMake 3.12 或更高版本
* cgroup v2，已启用 memory 和 pids controller
* 内核提供 `memory.swap.max`、`memory.peak`、`pids.max` 和 `cgroup.kill` 等当前代码使用的接口

运行前需要为当前用户配置可管理的 `/sys/fs/cgroup/minijudge/` 子树，并在该层启用 memory 和 pids controller。评测程序以普通用户身份运行，不能用 `sudo` 启动评测程序。

仓库提供 cgroup 环境配置脚本。脚本会检查 cgroup v2 以及 memory 和 pids controller，创建 `minijudge/manager`，启用 memory 和 pids controller，并将当前 shell 加入 `/sys/fs/cgroup/minijudge/manager`：

```bash
./scripts/setup-cgroup.sh
```

当前实现中，每次打开新的 shell 后，在运行 MiniJudge 前需要重新执行该脚本。脚本内部会在需要的位置请求管理员权限；MiniJudge 本身以普通用户身份运行，不应使用 `sudo` 启动。脚本需要在实际运行 MiniJudge 的 Linux 环境中执行。

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

使用默认 `1000 ms` 时间限制和 `64 MiB` 内存限制：

```bash
./build/minijudge examples/ac.cpp
```

自定义时间和内存限制：

```bash
./build/minijudge -t 1000 -m 64 examples/ac.cpp
./build/minijudge --time-limit 1000 --memory-limit 64 examples/ac.cpp
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

参数说明：

```text
-t, --time-limit <ms>      CPU 时间限制，单位 ms，默认 1000
-m, --memory-limit <MiB>   内存限制，单位 MiB，默认 64
-h, --help                显示帮助信息
```

时间限制和内存限制必须为正整数。非法参数、缺少源码路径或提供多个源码路径时，程序会输出错误并退出。

测试结果中显示的运行时间为测试程序所在 cgroup 的累计 CPU 时间。Wall Time 仅作为内部 watchdog，目前上限为 CPU Time Limit 的 3 倍。

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

1. 解析源码路径、时间限制和内存限制。
2. 扫描并校验测试数据，创建当前 MiniJudge 实例独立的临时工作目录。
3. `fork` 编译子进程，并通过 `execvp()` 执行 `g++`；编译器标准错误写入当前实例的 `compile.log`，编译失败输出 CE。
4. 根据硬件并发度创建固定数量的 Worker 线程。多个 Worker 通过共享索引和 `std::mutex` 动态领取待评测测试点。
5. 每个 Worker 为领取到的测试点创建独立实际输出文件和独立 cgroup。
6. `fork` 创建评测子进程；子进程加入对应 cgroup，通过 `dup2()` 重定向输入输出，再通过 `execv()` 执行用户程序。
7. 父进程使用 `waitpid(WNOHANG)` 轮询，并检查 cgroup `cpu.stat` 中的累计 CPU 时间与 Wall Time watchdog；超时后通过 `cgroup.kill` 终止用户程序及其后代进程，再使用 `waitpid()` 回收直接子进程。若整组终止失败，则使用 `SIGKILL` 兜底终止直接子进程。
8. 读取 `cpu.stat`、`memory.events` 中的 `oom_kill` 和 `memory.peak`，用于 CPU 时间统计、MLE 判定和峰值内存统计。
9. 使用 `cgroup.kill` 清理残留后代进程，等待 `cgroup.events` 中 `populated 0` 后删除测试点对应的 cgroup。
10. 无内部错误时，优先根据 OOM kill 判定 MLE，再根据 CPU / Wall 超时标记和退出状态判定 TLE / RE。
11. 正常退出且退出码为 0 时，使用内置 Checker 比较实际输出与标准答案，判定 AC / WA；Checker 自身失败时输出 `Judge failed`。
12. Worker 完成所有测试点后，主线程等待所有 Worker 结束，并按照测试点顺序统一输出状态、运行时间和峰值内存。

源码只编译一次，编译成功后由固定数量的 Worker 并行处理全部测试点。内部运行或资源管理失败显示 `Run failed`。

测试结果中的运行时间为测试程序所在 cgroup 的累计 CPU 时间，内存为清理前读取的 cgroup 峰值，输出单位为 MiB。实现细节见 [学习笔记](notes/learning-notes.md)。

### 运行与资源限制

每个测试点使用独立 cgroup。运行过程中 MiniJudge 周期性读取 `cpu.stat` 中的 `usage_usec`，统计测试程序及其后代进程的累计 CPU 时间。

当 CPU 时间超过 `-t` 指定的限制时判定 TLE。同时保留 3 倍时间限制的 Wall Time Watchdog，用于终止 `sleep`、阻塞或其他几乎不消耗 CPU 但长期不退出的程序。

测试结果中的时间为 CPU 时间，而非墙钟时间。

## 并行性能测试

为验证测试点并行调度效果，使用 55 个固定工作量的 CPU-bound 测试点进行本机测试。测试环境提供 20 个逻辑 CPU，每种模式运行 3 次，以下取中位数：

| 调度方式 | Worker 数量 | real | user | sys |
| --- | ---: | ---: | ---: | ---: |
| 串行评测 | 1 | 35.619 s | 32.759 s | 0.841 s |
| 无界并发 | 55 | 3.458 s | 42.136 s | 1.771 s |
| 固定 Worker | 20 | 3.442 s | 41.150 s | 0.870 s |

固定 Worker 相比串行评测将整批测试总耗时从约 `35.6 s` 降至 `3.44 s`，约为 `10.3×` 加速。

与一个测试点创建一个线程的无界并发相比，固定 Worker 的整体吞吐基本不变，同时系统态 CPU 时间从约 `1.77 s` 降至 `0.87 s`，下降约 `51%`。

无界并发还会使大量 CPU-bound 测试点同时竞争处理器，显著增加单个测试点的 wall time。固定 Worker 将活跃评测任务数量限制在接近硬件并发能力的范围内，在保持吞吐的同时减少额外调度竞争，并提高单测试点评测时间的稳定性。

以上数据为当前开发环境下的本机测试结果，不代表不同硬件和系统环境下的固定性能。

## 输出示例

以下仅展示输出格式，数值不是本次实测结果，各行可来自不同程序：

```text
Test 1: AC (2.314 ms, 1.203 MiB)
Test 2: WA (1.827 ms, 1.180 MiB)
Test 2#1: RE (11.318 ms, 1.180 MiB)
Test 300: TLE (1001.362 ms, 1.156 MiB)
Test abc: MLE (58.441 ms, 64.000 MiB)
```

编译失败：

```text
examples/ce.cpp CE
```

MiniJudge 内部运行错误：

```text
Test 1: Run failed (3.214 ms, 0.000 MiB)
```

## 临时文件

每个 MiniJudge 实例使用独立工作目录：

```text
tmp/run-<pid>/
```

其中包含：

```text
tmp/run-<pid>/compile.log
tmp/run-<pid>/user_program
tmp/run-<pid>/actual_<test_name>.out
```

正常评测结束后工作目录会被清理；部分异常退出场景下可能残留。

## 当前限制

* 必须从项目根目录运行
* 测试结果显示 cgroup 累计 CPU 时间；Wall Time 仅作为 CPU Time Limit 3 倍的 watchdog
* core dump 处理可能导致 RE 返回明显变慢
* 编译阶段仍通过外部 `g++` 命令完成
* 当前进程数上限固定为 64，尚不支持通过命令行配置
* 当前测试点并发数根据 `std::thread::hardware_concurrency()` 自动确定，尚不支持通过命令行手动指定 Worker 数量
* 当前 cgroup delegation 依赖 `scripts/setup-cgroup.sh`；新 shell 会话运行 MiniJudge 前需要重新执行该脚本
* 尚未实现完整 sandbox
* 尚未实现其他系统资源限制
* 当前进程被 `Ctrl+C` 等外部信号中断时，正常 cleanup 可能来不及执行，临时 cgroup 和工作目录可能残留
* 测试点按照字符串字典序运行

## 后续计划

* 完善 Runner 系统调用错误处理
* 进一步完善 CPU time 与 wall time watchdog 的边界处理，降低系统负载对 TLE 判定的影响
* 完善 cgroup 环境配置脚本的回滚、重复执行和错误处理
* 减少对 Shell 命令的依赖
* 完善测试集与项目文档
