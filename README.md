# MiniJudge

MiniJudge 是一个运行在 Linux 环境下的轻量级本地 C++ 代码评测工具。

支持编译待评测源码、自动发现测试点、批量运行程序、重定向输入输出并比较评测结果。

## 当前功能

* 通过命令行指定待评测的 C++ 源码
* 使用 `g++` 编译源码
* 将编译错误保存到 `tmp/compile.log`
* 自动扫描并校验 `tests/` 目录中的测试数据
* 批量运行多个测试点
* 重定向程序标准输入和标准输出
* 使用 `diff -wB` 比较实际输出与标准答案
* 输出以下基础结果：

  * `AC`：答案正确
  * `WA`：答案错误
  * `CE`：编译错误
  * `Run failed`：程序运行失败
* 使用 CMake 管理项目构建

当前版本使用 `std::system()` 执行编译、运行和比较命令。

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
│   └── TestCaseFinder.h
│
├── src/
│   ├── main.cpp
│   ├── Compiler.cpp
│   ├── Runner.cpp
│   ├── Checker.cpp
│   └── TestCaseFinder.cpp
│
├── examples/
├── tests/
├── tmp/
└── notes/
```

模块职责：

* `Compiler`：编译待评测源码
* `Runner`：运行程序并重定向输入输出
* `Checker`：比较实际输出与标准答案
* `TestCaseFinder`：发现并校验测试数据
* `main.cpp`：组织完整评测流程

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

## 运行项目

在项目根目录执行：

```bash
./build/minijudge <source_path>
```

例如：

```bash
./build/minijudge examples/accepted.cpp
```

未提供源码路径时，程序会输出：

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

测试点名称不要求是连续数字。

当前允许使用以下字符：

```text
A-Z  a-z  0-9  _  -  #  .
```

如果输入文件和标准答案未成对出现，程序会输出错误并停止评测。

其他扩展名的文件会被忽略。

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
    ├── 编译失败 ──► CE
    │
    ▼
运行测试点
    │
    ├── 运行失败 ──► Run failed
    │
    ▼
比较输出
    │
    ├── 输出相同 ──► AC
    └── 输出不同 ──► WA
```

源码只编译一次，编译成功后依次运行全部测试点。

## 输出示例

```text
Test 1: AC
Test 2: WA
Test 24#2: AC
```

运行失败时：

```text
Run failed on test 2
```

编译失败时：

```text
examples/compile_error.cpp CE
```

编译日志保存在：

```text
tmp/compile.log
```

实际输出保存在：

```text
tmp/actual_<test_name>.out
```

## 当前限制

* 必须从项目根目录运行
* 使用 `std::system()` 执行外部命令
* 不支持包含空格或 Shell 特殊字符的路径
* 无法精确区分不同类型的运行错误
* 尚未实现超时检测
* 尚未统计程序运行时间
* 尚未实现内存和其他资源限制
* 测试点按照字符串字典序运行

## 后续计划

* 使用 `fork()`、`exec()`、`dup2()` 和 `waitpid()` 管理评测进程
* 统计每个测试点的运行时间
* 实现超时检测和进程终止
* 精确判断运行时错误
* 增加内存和其他资源限制
* 生成结构化评测报告
