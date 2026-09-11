# MiniJudge

MiniJudge 是一个运行在 Linux 环境下的轻量级本地 C++ 代码评测工具。

> 🌾 **100% 古法编程** · 手搓 · 手调 · 手测 ( •̀ ω •́ )✧

支持编译待评测源码、自动发现测试点、批量运行程序、输入输出重定向、运行时间统计、峰值内存统计以及 AC、WA、CE、RE、TLE、MLE 判定。

## 当前功能

* 通过命令行指定待评测的 C++ 源码
* 支持通过 `-t` / `--time-limit` 自定义时间限制，默认 `1000 ms`
* 支持通过 `-m` / `--memory-limit` 自定义内存限制，默认 `64 MiB`
* 支持 `-h` / `--help` 查看命令行帮助
* 使用 `fork()` + `execvp()` 调用 `g++` 编译源码
* 使用 `dup2()` 将编译器标准错误重定向到 `tmp/compile.log`
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
* 通过 `memory.events` 的 `oom_kill` 判断 MLE
* 通过 `memory.peak` 统计测试点峰值内存
* 使用 `cgroup.kill` 清理残留后代进程，等待 `populated 0` 后删除控制组
* 统计每个测试点运行时间和峰值内存
* 内置输出比较器：忽略空行和行末空白，其余内容逐字符比较
* 使用 CMake 管理项目构建

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
├── tests/
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
* `main.cpp`：解析命令行参数并组织完整评测流程

`build/` 和 `tmp/` 中生成的临时文件不会提交到 Git 仓库。

## 环境要求

* Linux
* g++，支持 C++17
* CMake 3.12 或更高版本
* cgroup v2，已启用 memory controller
* 内核提供 `memory.swap.max`、`memory.peak` 和 `cgroup.kill` 等当前代码使用的接口

运行前需要为当前用户配置可管理的 `/sys/fs/cgroup/minijudge/` 子树，并在该层启用 memory controller。评测程序以普通用户身份运行，不能用 `sudo` 启动评测程序。

仓库提供 cgroup 环境配置脚本。脚本会检查 cgroup v2 和 memory controller，创建 `minijudge/manager`，启用 memory controller，并将当前 shell 加入 `/sys/fs/cgroup/minijudge/manager`：

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
-t, --time-limit <ms>      时间限制，单位 ms，默认 1000
-m, --memory-limit <MiB>   内存限制，单位 MiB，默认 64
-h, --help                显示帮助信息
```

时间限制和内存限制必须为正整数。非法参数、缺少源码路径或提供多个源码路径时，程序会输出错误并退出。

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
2. 扫描并校验测试数据，`fork` 编译子进程并通过 `execvp()` 执行 `g++`；编译器标准错误重定向到 `tmp/compile.log`，编译失败输出 CE。
3. 为当前测试点创建 `/sys/fs/cgroup/minijudge/run`，设置内存限制并禁用 swap。
4. `fork` 创建评测子进程；子进程先加入 cgroup，再通过 `dup2` 重定向输入输出、`execv` 执行用户程序。
5. 父进程使用 `waitpid(WNOHANG)` 轮询，并检查 wall time；超时后通过 `cgroup.kill` 终止用户程序及其后代进程，再使用 `waitpid()` 回收直接子进程。若整组终止失败，则使用 `SIGKILL` 兜底终止直接子进程。检测到 core dump 后暂缓超时终止，以处理 RE/TLE 误判。
6. 读取 `memory.events` 中的 `oom_kill` 和 `memory.peak`。
7. 写入 `cgroup.kill` 清理残留后代进程，等待 `cgroup.events` 的 `populated 0`，再删除 cgroup。
8. 无内部错误时，优先根据 OOM kill 判定 MLE，再根据超时标记和退出状态判定 TLE / RE。
9. 正常退出且退出码为 0 时，使用内置 Checker 比较实际输出与标准答案，判定 AC / WA；Checker 自身失败时输出 `Judge failed`。
10. 输出当前测试点的状态、运行时间和峰值内存。

源码只编译一次，编译成功后依次运行全部测试点。内部运行或资源管理失败显示 `Run failed`。

运行时间为 Runner 从开始到返回的 wall time，包含控制组创建、等待和清理开销；内存为清理前读取的 cgroup 峰值，输出单位为 MiB。实现细节见 [学习笔记](notes/learning-notes.md)。

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
* 尚未实现 CPU Time 限制
* 当前 cgroup delegation 依赖 `scripts/setup-cgroup.sh`；新 shell 会话运行 MiniJudge 前需要重新执行该脚本
* 测试点使用固定的 `run` cgroup 和临时文件路径，不支持并行评测或多个实例同时运行
* 尚未实现完整 sandbox
* 尚未实现其他系统资源限制
* 测试点按照字符串字典序运行

## 后续计划

* 完善 Runner 系统调用错误处理
* 区分 wall time 与 CPU time
* 增加 CPU time 等其他资源限制
* 完善 cgroup 环境配置脚本的回滚、重复执行和错误处理
* 减少对 Shell 命令的依赖
* 完善测试集与项目文档
