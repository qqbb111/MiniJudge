# MiniJudge

MiniJudge 是一个运行在 Linux 环境下的轻量级本地 C++ 代码评测工具。

项目实现了代码编译、测试点运行、输入输出重定向以及答案比较等基础评测流程，可用于在本地批量运行 C++ 程序并输出评测结果。

## 当前功能

- 编译待评测的 C++ 源代码
- 将编译错误保存到日志文件
- 批量运行多个测试点
- 从 `.in` 文件重定向标准输入
- 将程序输出保存到临时文件
- 使用 `diff` 比较实际输出与标准答案
- 输出以下基础评测结果：
  - `AC`：答案正确
  - `WA`：答案错误
  - `CE`：编译错误
  - `Run failed`：程序运行失败
- 使用 CMake 管理项目构建

当前版本使用 `std::system()` 执行编译、运行和比较命令，尚未实现精确的运行状态识别和资源限制。

## 项目结构

```text
MiniJudge/
├── CMakeLists.txt          # CMake 构建配置
├── README.md               # 项目说明
├── .gitignore              # Git 忽略规则
│
├── include/                # 模块接口声明
│   ├── Compiler.h
│   ├── Runner.h
│   └── Checker.h
│
├── src/                    # 模块实现
│   ├── main.cpp            # 组织评测流程
│   ├── Compiler.cpp        # 编译待评测代码
│   ├── Runner.cpp          # 运行待评测程序
│   └── Checker.cpp         # 比较实际输出与标准答案
│
├── examples/               # 待评测的 C++ 示例代码
├── tests/                  # 测试输入和标准答案
├── tmp/                    # 评测过程中生成的临时文件
└── notes/                  # 项目开发笔记
```

`build/` 是 CMake 自动生成的构建目录，不提交到 Git 仓库。

## 环境要求

- Linux
- g++，支持 C++17
- CMake 3.10 或更高版本
- `diff`

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

构建成功后，可执行文件位于：

```text
build/minijudge
```

## 运行项目

在项目根目录执行：

```bash
./build/minijudge
```

当前版本使用相对路径访问 `examples/`、`tests/` 和 `tmp/`，因此需要从项目根目录启动程序。

示例输出：

```text
Test1: Run OK AC
Test2: Run OK WA
```

编译失败时可能输出：

```text
examples/compile_err.cpp CE
```

## 测试数据格式

每个测试点由一个输入文件和一个标准答案文件组成：

```text
tests/
├── 1.in
├── 1.out
├── 2.in
└── 2.out
```

其中：

- `1.in`：第一个测试点的输入
- `1.out`：第一个测试点的标准答案
- `2.in`：第二个测试点的输入
- `2.out`：第二个测试点的标准答案

当前测试点数量和待评测源码路径在 `src/main.cpp` 中配置。

## 评测流程

```text
待评测源码
    │
    ▼
Compiler
    │
    ├── 编译失败 ──► CE
    │
    ▼
生成可执行文件
    │
    ▼
Runner
    │
    ├── 运行失败 ──► Run failed
    │
    ▼
生成实际输出
    │
    ▼
Checker
    │
    ├── 输出相同 ──► AC
    └── 输出不同 ──► WA
```

### 编译

MiniJudge 调用类似下面的命令编译用户代码：

```bash
g++ examples/accepted.cpp \
    -std=c++17 \
    -O2 \
    -o tmp/user_program \
    2> tmp/compile.log
```

编译错误会写入：

```text
tmp/compile.log
```

### 运行

用户程序通过输入输出重定向运行：

```bash
./tmp/user_program < tests/1.in > tmp/actual1.out
```

### 比较

实际输出与标准答案通过 `diff` 比较：

```bash
diff -wB tmp/actual1.out tests/1.out
```

参数含义：

- `-w`：忽略空白字符差异
- `-B`：忽略空白行

## 模块说明

### Compiler

负责调用 g++ 编译待评测源码，并保存编译错误日志。

接口：

```cpp
bool compile(const std::string& code,
             const std::string& exe);
```

### Runner

负责运行编译后的程序，并完成输入输出重定向。

接口：

```cpp
bool run(const std::string& exe,
         const std::string& input,
         const std::string& output);
```

### Checker

负责比较实际输出和标准答案。

接口：

```cpp
bool compare(const std::string& actual,
             const std::string& expected);
```

## 当前限制

- 待评测源码路径暂时写在 `src/main.cpp` 中
- 测试点数量暂时写在 `src/main.cpp` 中
- 使用 `std::system()` 执行外部命令
- 无法精确区分不同类型的运行错误
- 尚未实现超时检测
- 尚未实现内存限制
- 尚未统计程序运行时间
- 当前路径拼接方式不支持包含空格的文件路径

## 后续计划

- 通过命令行参数指定源码和测试数据路径
- 自动发现测试目录中的测试点
- 使用 `fork()`、`exec()` 和 `waitpid()` 管理评测进程
- 精确判断运行时错误
- 实现时间限制和超时终止
- 统计每个测试点的运行时间
- 增加资源限制
- 生成结构化评测报告
