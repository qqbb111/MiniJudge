# MiniJudge 学习笔记

# C++ 基础

## 1. `const T&` 传参

只读的 `std::string`、`std::vector` 等对象通常使用常量引用传参：

```cpp
bool run(const std::string& path);
```

```text
T        复制对象
T&       引用对象，可以修改
const T& 引用对象，不可修改
```

作用：避免复制，并防止函数修改原对象。

常见坑：小型基础类型如 `int` 通常直接按值传递，不必使用引用。

---

## 2. `std::string::c_str()`

`c_str()` 将 `std::string` 转换为 C 接口需要的 `const char*`：

```cpp
std::string cmd = "ls";
std::system(cmd.c_str());
```

常见场景：`std::system()` 等传统 C 接口不能直接接收 `std::string`。

---

## 3. 结构体保存一组结果

当函数需要返回多个相关数据时，可以定义结构体：

```cpp
struct RunResult {
    bool success;
    long long elapsedMicroseconds;
};
```

返回时可以使用聚合初始化：

```cpp
return {res == 0, elapsedUs};
```

当前含义：

```text
success             运行命令是否成功
elapsedMicroseconds 本次运行的墙上时间，单位为微秒
```

常见坑：结构体应在确实需要保存或传递一组相关数据时再引入，不要为了使用新语法过早抽象。

---

## 4. 项目命名规则

当前 MiniJudge 统一使用：

```text
类型名       PascalCase       RunResult、TestCase
函数和变量   lowerCamelCase   findTestCases、sourcePath
模块文件名   PascalCase       Runner.h、Runner.cpp
程序入口     固定惯例         main.cpp
```

命名风格没有唯一标准，项目内部一致最重要。

---

# Linux 命令与重定向

## 5. `system()`

`std::system()` 让程序调用 Shell 执行命令：

```cpp
std::string cmd = "ls";
int res = std::system(cmd.c_str());
```

当前阶段约定：

```text
res == 0  命令成功
res != 0  命令失败
```

限制：当前不能精确区分程序崩溃、命令不存在、具体退出原因等情况。

后续将使用：

```text
fork
exec
dup2
waitpid
```

替代 `system()`。

---

## 6. Linux 标准输入输出

文件描述符：

```text
0  stdin   标准输入
1  stdout  标准输出
2  stderr  标准错误
```

重定向：

```bash
program < input.txt
program > output.txt
program 2> error.log
```

分别表示从文件读取标准输入、把标准输出写入文件、把标准错误写入文件。

---

## 7. `diff` 比较

当前命令：

```bash
diff -wB actual.out expected.out
```

参数：

```text
-w  忽略空白字符差异
-B  忽略空白行
```

返回值：

```text
0   文件相同
1   文件不同
>1  diff 执行错误
```

---

## 8. `/dev/null`

`/dev/null` 是 Linux 的黑洞文件，写入内容会被丢弃：

```bash
command > /dev/null
```

用途：隐藏不需要的标准输出。

---

# MiniJudge 基础流程

## 9. `compile()`

职责：

```text
调用 g++ 编译源码
生成可执行文件
保存编译错误日志
返回编译是否成功
```

接口：

```cpp
bool compile(
    const std::string& code,
    const std::string& exe
);
```

实际命令：

```bash
g++ source.cpp -std=c++17 -O2 -o program 2> tmp/compile.log
```

常见坑：不能根据可执行文件是否存在判断编译成功，因为旧文件可能残留。

---

## 10. `run()`

职责：

```text
运行用户程序
把测试输入重定向到 stdin
把程序输出重定向到文件
统计运行时间
返回运行状态和耗时
```

数据流：

```text
输入文件
   ↓
 stdin
   ↓
用户程序
   ↓
 stdout
   ↓
实际输出文件
```

命令：

```bash
./program < test.in > actual.out
```

接口返回：

```cpp
RunResult run(
    const std::string& exePath,
    const std::string& inputPath,
    const std::string& actualOutputPath
);
```

当前限制：`system()` 非零只能统一判断为 `Run failed`，无法精确区分 RE 和 TLE。

---

## 11. `compare()`

职责：比较实际输出和标准答案。

```text
actual output
      vs
expected output
```

当前使用 `diff -wB`，返回比较是否通过。

完整流程：

```text
compile
   ↓
run + timing
   ↓
compare
   ↓
AC / WA / CE / Run failed
```

---

# 运行时间统计

## 12. `std::chrono::steady_clock`

需要头文件：

```cpp
#include <chrono>
```

`steady_clock` 是单调递增的时钟，适合测量时间间隔，不受系统日期时间调整影响。

```cpp
auto start = std::chrono::steady_clock::now();
// 待测操作
auto end = std::chrono::steady_clock::now();
```

`now()` 返回当前时钟上的一个时间点；两个时间点相减得到时间长度：

```cpp
auto elapsed = end - start;
```

常见坑：测量程序耗时应使用 `steady_clock`，不要依赖可能发生跳变的系统日历时间。

---

## 13. `duration_cast`

`end - start` 的底层单位由时钟实现决定，需要显式转换：

```cpp
long long elapsedUs =
    std::chrono::duration_cast<std::chrono::microseconds>(
        end - start
    ).count();
```

```text
duration_cast  转换时间单位
microseconds   目标单位为微秒
count()         取出数值
```

当前使用微秒保存，避免极短程序直接转换为整数毫秒后显示为 `0`。

---

## 14. Runner 的计时范围

当前顺序：

```text
构造命令
记录 start
调用 system()
记录 end
计算耗时
返回 RunResult
```

Checker 的 `diff` 不计入 Runner 的运行时间。

即使运行失败，也应保留已经测得的耗时：

```cpp
return {res == 0, elapsedUs};
```

常见坑：失败分支直接返回 `{false, 0}` 会丢失真实耗时。

---

## 15. 当前时间是墙上时间

在 `system()` 前后计时得到的是完整墙上时间，包括：

```text
启动 Shell
解析命令
创建进程
打开输入输出文件
执行重定向
运行用户程序
等待进程结束
```

因此极短程序也可能显示数毫秒，并且在虚拟机中存在明显波动。

不能通过预运行再减去所谓“空跑时间”消除开销，因为每次调度和缓存状态不同，额外运行还会改变程序状态。

---

## 16. `std::fixed` 与 `std::setprecision`

需要头文件：

```cpp
#include <iomanip>
```

统一输出三位小数：

```cpp
std::cout << std::fixed << std::setprecision(3);
```

配合输出：

```cpp
std::cout << elapsedMicroseconds / 1000.0 << " ms";
```

说明：

```text
fixed           使用定点小数形式
setprecision(3) 保留 3 位小数
```

格式设置会持续作用于同一个输出流，不必在每个分支重复设置。

常见坑：没有 `fixed` 时，`setprecision(3)` 表示三位有效数字，不是三位小数。

---

# Git

## 17. Git 基本流程

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

## 18. `git init`

初始化 Git 仓库：

```bash
git init
```

生成 `.git/`，用于保存版本信息。

---

## 19. `.gitignore`

指定不提交的文件：

```gitignore
tmp/*
!tmp/.gitkeep
```

---

## 20. `git commit`

保存一次版本：

```bash
git commit -m "feat: implement basic judge flow"
```

当前提交信息格式：

```text
type: description
```

常用类型：

```text
feat  新功能
fix   修复问题
docs  文档更新
refactor 代码重构
```

---

## 21. 查看历史

```bash
git log --oneline
git show <commit_id>
git show --stat <commit_id>
```

---

## 22. 修改最近一次提交

最近一次提交尚未推送，并且需要补充遗漏内容：

```bash
git add <files>
git commit --amend --no-edit
```

提交哈希会改变。

---

## 23. 撤回提交但保留代码

```bash
git reset --soft HEAD~1
```

结果：最近一次提交被撤回，代码仍保留在暂存区。

常见坑：`git reset --hard` 会丢弃工作区修改，不应随意使用。

---

# C++ 多文件项目

## 24. `src` 与 `include`

```text
src/      .cpp 实现文件
include/  .h 接口文件
```

`.h` 负责声明接口，`.cpp` 负责实现功能。

---

## 25. `#pragma once`

写在头文件顶部，防止头文件被重复包含：

```cpp
#pragma once
```

---

## 26. 多文件编译

```bash
g++ src/main.cpp src/Compiler.cpp \
    -Iinclude \
    -std=c++17 \
    -Wall \
    -Wextra \
    -o tmp/minijudge
```

多个 `.cpp` 会分别编译，最后链接为一个可执行文件。

---

## 27. 模块拆分原则

模块按职责拆分，不采用“一函数一个 `.h/.cpp`”。

当前职责：

```text
Compiler          编译用户程序
Runner            运行、重定向和计时
Checker           比较输出
TestCasesFinder   发现并校验测试数据
main              组织完整评测流程
```

新功能优先并入已有职责明确的模块，只有出现新的独立职责时才考虑新增模块。

---

# C++ 命名空间

## 28. `std::`

`std` 是 C++ 标准库命名空间：

```cpp
std::string
std::cout
std::vector
```

---

## 29. `using namespace std`

使用后可以省略 `std::`，但工程代码中建议显式写出标准库命名空间。

尤其不要在头文件中写：

```cpp
using namespace std;
```

因为它会影响所有包含该头文件的源文件。

---

# GCC 编译参数

## 30. `-Wall` 与 `-Wextra`

```text
-Wall   开启常见编译警告
-Wextra 开启更多额外警告
```

通常一起使用：

```bash
-Wall -Wextra
```

警告不等于编译错误，但应认真检查。

---

# CMake

## 31. `CMakeLists.txt`

CMake 统一管理项目的编译和链接：

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
target_include_directories(minijudge PRIVATE include)
```

这样源码中可以直接写：

```cpp
#include "Runner.h"
```

常见坑：新增 `.cpp` 后没有加入 `CMakeLists.txt`，可能出现 `undefined reference`。

---

## 32. CMake 构建命令

配置项目：

```bash
cmake -S . -B build
```

编译项目：

```bash
cmake --build build
```

```text
-S .        源码目录是当前目录
-B build    构建文件放入 build/
--build     执行实际编译和链接
```

---

# 命令行参数

## 33. `argc` 与 `argv`

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
argc == 2
argv[0] == "./build/minijudge"
argv[1] == "examples/accepted.cpp"
```

使用前必须检查：

```cpp
if (argc < 2) {
    std::cerr << "Usage: "
              << argv[0]
              << " <source_path>\n";
    return 1;
}
```

常见坑：检查 `argc` 前不能访问 `argv[1]`。

---

# C++ 文件系统

## 34. `std::filesystem`

需要：

```cpp
#include <filesystem>
```

常用判断：

```cpp
std::filesystem::exists(path);
std::filesystem::is_directory(path);
```

路径存在不代表它一定是目录。

---

## 35. 遍历目录

```cpp
for (const auto& entry :
     std::filesystem::directory_iterator(testDir)) {
    if (!entry.is_regular_file()) {
        continue;
    }
}
```

`directory_iterator` 依次访问目录项，但遍历顺序没有保证。

---

## 36. `path` 文件名操作

```cpp
std::filesystem::path path = entry.path();

std::string filename = path.filename().string();
std::string stem = path.stem().string();
std::string extension = path.extension().string();
```

示例：

```text
文件名     stem    extension
1.in       1       .in
24#2.out   24#2    .out
.in        .in     空字符串
```

`.in` 是 Linux 隐藏文件名，不是空主名加 `.in` 扩展名，因此需要单独检查完整文件名。

---

## 37. 文件系统路径拼接

推荐：

```cpp
std::filesystem::path input =
    std::filesystem::path(testDir) / (name + ".in");
```

传给接收 `std::string` 的函数：

```cpp
input.string()
```

常见坑：手动拼接字符串会依赖目录末尾是否带 `/`。

---

# 测试点自动发现

## 38. 测试点配对规则

同名的 `<name>.in` 和 `<name>.out` 构成一个测试点：

```text
1.in       1.out
24#2.in    24#2.out
sample.in  sample.out
```

测试名不要求是连续整数。

---

## 39. 使用 `set` 保存测试名

```cpp
std::set<std::string> inputNames;
std::set<std::string> outputNames;
```

检查配对：

```cpp
if (outputNames.count(name) == 0) {
    // 缺少标准答案
}
```

需要双向检查输入和输出，不能只统计文件数量再除以二。

生成列表：

```cpp
testNames.assign(inputNames.begin(), inputNames.end());
```

`std::set<std::string>` 按字符串字典序排列：

```text
1 → 10 → 2 → 24#2
```

---

## 40. 测试名白名单

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

使用白名单是因为路径会被拼接到 Shell 命令中，需要避免空格和特殊字符改变命令含义。

常见坑：应先确认扩展名是 `.in` 或 `.out`，再校验测试名，避免无关文件被误判。

---

## 41. `findTestCases()`

接口：

```cpp
bool findTestCases(
    const std::string& testDir,
    std::vector<std::string>& testNames,
    std::string& errMessage
);
```

参数：

```text
testDir     输入：测试目录
testNames   输出：测试点名称
errMessage  输出：失败原因
返回值      是否成功
```

函数开始时清空输出参数：

```cpp
testNames.clear();
errMessage.clear();
```

失败后，调用方不能继续使用 `testNames` 进行评测。

---

# GitHub

## 42. Remote 与 Push

添加远程仓库：

```bash
git remote add origin <repository_url>
```

查看：

```bash
git remote -v
```

第一次推送：

```bash
git push -u origin main
```

以后：

```bash
git push
```

---

# 工程设计结论

## 43. 不要过早抽象

只有数据确实需要保存、传递、统计或统一处理时，才增加额外类型或转换函数。

当前运行结果需要同时返回状态和耗时，因此 `RunResult` 有明确价值；尚未需要统一保存所有评测结果，因此暂时不增加复杂报告类型。

---

# MiniJudge 当前状态

## v0.1 基础评测流程

```text
compile
→ run
→ compare
→ AC / WA / CE / Run failed
```

## v0.2 模块拆分

```text
Compiler
Runner
Checker
main
```

## v0.3 CMake 构建

```bash
cmake -S . -B build
cmake --build build
```

## v0.4 命令行源码路径

```bash
./build/minijudge examples/accepted.cpp
```

## v0.5 测试点自动发现

```text
扫描 tests/
校验 .in / .out 配对
校验测试名
返回测试点列表
```

## v0.6 每个测试点运行时间统计

```text
Runner 返回 RunResult
steady_clock 测量墙上时间
微秒保存
毫秒显示并保留三位小数
AC / WA / Run failed 均输出耗时
```

当前项目结构：

```text
include/
    Compiler.h
    Runner.h
    Checker.h
    TestCasesFinder.h

src/
    main.cpp
    Compiler.cpp
    Runner.cpp
    Checker.cpp
    TestCasesFinder.cpp
```

---

# 当前尚未完成

```text
TLE 检测和进程终止
精确区分运行时错误
文件系统异常处理
Shell 参数完整转义
支持带空格的路径
修复必须从项目根目录运行的问题
fork / exec / dup2 / waitpid
CPU 时间、内存和其他资源限制
结构化评测报告
```
