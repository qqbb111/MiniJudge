# cgroup v2：资源限制、统计与后代进程清理

这篇笔记围绕 MiniJudge 当前的 `Cgroup.cpp`、`Runner.cpp` 和环境配置脚本展开。原来的综合笔记保留在 [notes/learning-notes.md](../../notes/learning-notes.md)。

## 1. 我为什么需要它

Runner 通过 `fork + exec` 启动一个提交，但提交也可以继续 `fork` 或创建线程。只记录最初那个 PID，无法把它的整个工作负载当成一个整体限制和统计。

`kill(pid, SIGKILL)` 只针对指定进程；`waitpid(pid, ...)` 只等待并回收可等待的子进程，也不会自动杀掉它的后代。MiniJudge 需要一个能承载整个测试点的资源分组。

cgroup（control group，控制组）把进程组织成树，通过 `/sys/fs/cgroup/` 下的接口设置限制、读取统计。目录是内核提供的控制接口，不是普通存储目录。MiniJudge 为每个测试点创建一个独立控制组，提交在 `exec` 前加入，之后创建的后代继承该组归属。

## 2. 运行模型

```text
/sys/fs/cgroup/
└── minijudge/                       已授权给当前用户，启用 memory / pids
    ├── manager/                     当前 shell、MiniJudge、编译器
    ├── run-<judge_pid>-1/            测试点 1 的提交及其后代
    └── run-<judge_pid>-2/            测试点 2 的提交及其后代
```

MiniJudge 本身留在 `manager`，每个评测子进程移入自己的 `run-*`。因此提交的资源统计不会把评测器和编译器算进去；不同测试点也不会共用一次 OOM 计数或内存峰值。

生命周期：

```text
createCgroup -> 设置限制 -> fork
                            |
                            + 子进程：joinCgroup -> dup2 -> execv
                            |
                            + 父进程：waitpid(WNOHANG) + CPU / wall 监控
                                      -> 读取 OOM 和峰值内存
                                      -> cgroup.kill
                                      -> 等待 populated 0
                                      -> 删除控制组
                                      -> 整理运行状态
```

超时分支会提前整组终止，再 `waitpid` 回收直接子进程。运行结束后的清理还会处理可能仍活着的后代。

## 3. MiniJudge 中对应的代码

### 环境配置：为什么每个新 shell 都要执行

[`scripts/setup-cgroup.sh`](../../scripts/setup-cgroup.sh) 检查 cgroup v2 和根层的 memory controller，创建 `minijudge/manager`，把调用脚本的 shell（`$PPID`）放进 `manager`，在 `minijudge/cgroup.subtree_control` 中启用 `memory`、`pids`，再把运行需要的父层目录和迁移接口授权给当前用户。

启用 controller 后，子组才会提供 `memory.max` 等接口。将 shell 放进 `manager` 也让 `minijudge` 父层不直接承载工作进程，满足当前树形结构的需要。

```bash
# 在真正运行 MiniJudge 的 Bash 中执行
./scripts/setup-cgroup.sh
./build/minijudge -j 1 examples/ac.cpp
```

一个新 shell 可能还在 `/init.scope` 等别的组里，不能把“目录已经存在”当成“当前 shell 已正确授权”。非特权进程迁移还依赖源组、目标组及公共祖先的权限。脚本必须在同一 shell 中先执行；不要用 `sudo` 跑提交来绕开授权。

脚本目前没有单独预检 pids controller，也没有完整回滚。如果执行失败，应检查实际报错、`cgroup.controllers` 和父层 `cgroup.subtree_control`，不能只看目录是否存在。

### 创建并设置限制

[`createCgroup()`](../../src/Cgroup.cpp) 新建组后写入：

```cpp
writeControlFile(cgroupPath / "memory.max", memoryLimitBytes);
writeControlFile(cgroupPath / "memory.swap.max", 0);
writeControlFile(cgroupPath / "pids.max", 64);
```

| 接口 | 含义 | 当前用法 |
| --- | --- | --- |
| `memory.max` | 内存硬上限，单位 bytes | `-m` 的 MiB 转为 bytes |
| `memory.swap.max` | swap 使用上限 | 写入 0 |
| `pids.max` | 任务数量上限，包含线程 | 固定 64 |

这些是控制组总量约束，不是每个后代各享有一份配额。`memory.max` 也不是虚拟地址空间限制，不能把它解释成 `RLIMIT_AS`。

### 加入控制组：为什么写入 0

[`joinCgroup()`](../../src/Cgroup.cpp) 用 `open + write` 向目标 `cgroup.procs` 写入：

```cpp
write(fd, "0\n", 2);
```

在这个接口中，0 表示调用者自己。调用发生在评测子进程中，所以移动的是将要 `execv` 的子进程，父进程继续留在 `manager`。`exec` 替换程序映像，不会自动换一个 PID 或离开控制组。

加入控制组失败属于评测器内部错误。Runner 通过错误管道向父进程报告，不能把它等同于用户程序的 RE。

### CPU time 与 wall time

Runner 周期性读取 `cpu.stat` 中的 `usage_usec`：

```cpp
bool cpuTimeExceeded = cpuUsageUs > timeLimitMs * 1000;
bool wallTimeExceeded = wallElapsedUs > timeLimitMs * 3000;
```

CPU time（CPU 时间）是组内进程实际消耗的累计 CPU 时间；wall time（墙钟时间）是从启动到当前经历的时间。多核并行时，累计 CPU 时间可以大于墙钟时间。

`sleep` 消耗很少的 CPU，却可以一直不退出，所以还要 3 倍阈值的 watchdog。当前实现大约每 3 ms 轮询，不能保证在阈值瞬间终止。

代码检测到 `CoreDumping` 后会保持标记并跳过超时终止，以缓解 RE 被误判为 TLE 的问题；这也意味着该路径没有 watchdog 的同等保护。超时分支退出后，目前没有再刷新最终 CPU 使用量。

### MLE 和峰值内存

`readOomKillCount()` 读取 `memory.events` 的 `oom_kill`，`readMemoryPeak()` 读取 `memory.peak`。一个新测试点使用一个新组，因此不需要跨测试点复用旧计数。

当前判定顺序大体为：内部错误 → OOM kill / MLE → 已记录的超时 / TLE → 信号或非零退出码 / RE → 正常结束 / Checker。

超过 `memory.max` 可能先触发回收或分配失败；**当前代码以 `oom_kill > 0` 作为 MLE 依据**，不会把所有分配失败都直接判为 MLE。

`memory.peak` 是控制组计费内存的峰值，不能简单等同于主进程 RSS。当前读取发生在最后一次整组清理之前，尚存活后代在之后产生的消耗不一定包含在已读取结果中。

### 清理：kill 与 reap 是两件事

[`killCgroup()`](../../src/Cgroup.cpp) 向 `cgroup.kill` 写 1，然后轮询 `cgroup.events` 中的 `populated`：

```text
cgroup.kill = 1   -> 请求终止整个组的进程
populated = 0     -> 组及其子树没有活跃进程
removeCgroup()   -> 删除这个控制组目录
```

`cgroup.kill` 不代替 `waitpid`：终止工作负载与回收直接子进程的退出状态是两件事。超时时 Runner 两者都做；若整组终止失败，则用 `SIGKILL` 尝试兜底终止直接子进程，这种兜底不能保证所有后代已被清理。

清理循环每次重新读取 `cgroup.events`，约每 1 ms 轮询一次，目前没有独立截止时间。不能把“写入 kill 成功”解释为“目录立刻可删”，也不能把实现说成在所有异常情况下都保证清理成功。

## 4. 容易搞错的地方：pids.max 实验

[`examples/fork64.cpp`](../../examples/fork64.cpp) 用一个 pipe（管道）让成功创建的子进程保持存活：子进程关闭写端后阻塞读取，父进程完成创建尝试后关闭写端，子进程得到 EOF 并退出，父进程逐个 `waitpid`，最后输出输入两数之和。

在当前固定 `pids.max = 64`、提交没有额外线程的条件下，预期现象是：

```text
1 个提交主进程 + 63 个仍存活的子进程 = 64 个任务
第 64 次创建 child 的 fork 返回 -1，errno 为 EAGAIN
程序处理失败、释放子进程、正确输出答案，仍可 AC
```

2026-10-04 发布前在 WSL2 Ubuntu 24.04 中，以普通用户从 `manager` 启动 MiniJudge，使用单个 `10 20` → `30` 测试点验证：stderr 出现 `fork #64 failed` 和 `Successfully created 63 children`，最终 verdict 为 AC，测试点 cgroup 正常删除。

复现命令：

```bash
./scripts/setup-cgroup.sh
./build/minijudge -j 1 examples/fork64.cpp
```

`-j 1` 便于阅读示例写到 stderr 的诊断。限制针对每个测试点自己的组，其他测试点的进程不占用这一组的 64 个名额。若机器还有更严格的祖先组限制或其他系统限制，也可能更早失败，应结合实际日志判断。

可在另一终端中找到运行中的 `run-*` 目录，读取 `pids.current` 和 `pids.events`。示例结束得很快，需要观察窗口；也可以在实验副本中临时延长保持时间。

`pids.events` 的 `max` 是限制事件计数，不是当前存活任务数。MiniJudge 当前不读取它来决定 verdict，也没有单独的 Process Limit Exceeded 状态。是否 AC / RE / TLE 仍取决于提交怎样处理失败以及最终结果。

## 5. 面试怎么回答

> MiniJudge 的一个提交可以继续创建子进程，所以单个 PID 不足以覆盖整个测试点。我为每个测试点创建独立的 cgroup v2，在 exec 前让子进程加入，之后的后代继承组归属。通过 memory.max 和 pids.max 约束资源，通过 cpu.stat 统计累计 CPU 时间，同时用墙钟 watchdog 处理 sleep 和阻塞。运行结束读取 OOM 事件和峰值内存，再整组终止残留进程、等待组清空并删除目录。waitpid 则负责回收直接子进程。

可以继续追问自己：

- 为什么编译器和 MiniJudge 本身留在 `manager`？
- 进程被杀掉后，为什么仍需要 `waitpid`？
- `fork` 因 pids 上限失败后，程序为什么仍可能 AC？
- 为什么 `sleep` 需要 wall time watchdog？
- 为什么 cgroup 资源限制不等于完整安全沙箱？

这套实现的边界也要能解释：没有系统调用、文件系统或网络隔离；清理等待没有独立超时；外部信号中断时可能残留资源。会限制和统计资源，并不意味着已经能安全执行任意不可信代码。
