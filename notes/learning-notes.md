# MiniJudge 学习笔记

------------------------------------------------------------------------

# C++ 基础

## 1. `const T&` 传参

结论：

只读的 `string`、`vector` 等对象通常使用：

``` cpp
bool run(const string& path);
```

原因：

-   避免复制，提高效率
-   防止函数修改原对象

区别：

``` cpp
T        // 复制一份
T&       // 引用，可以修改
const T& // 引用，不可修改
```

------------------------------------------------------------------------

## 2. `std::string::c_str()`

作用：

把 `std::string` 转成 C 接口需要的 `const char*`。

例如：

``` cpp
string cmd = "ls";
system(cmd.c_str());
```

常见场景：

``` cpp
system()
```

需要：

``` cpp
const char*
```

而不是：

``` cpp
string
```

------------------------------------------------------------------------

# Linux 命令与重定向

## 3. system()

作用：

让程序调用 shell 执行命令。

示例：

``` cpp
string cmd = "ls";
int res = system(cmd.c_str());
```

当前阶段：

``` text
返回 0：
认为成功

非 0：
认为失败
```

注意：

`system()` 无法精确区分：

-   编译错误
-   程序崩溃
-   命令不存在

后续使用：

``` text
fork
exec
waitpid
```

替代。

------------------------------------------------------------------------

## 4. Linux 标准输入输出

文件描述符：

``` text
0 : stdin 标准输入
1 : stdout 标准输出
2 : stderr 标准错误
```

重定向：

``` bash
< input.txt
```

标准输入来自文件。

``` bash
> output.txt
```

标准输出写入文件。

``` bash
2> error.log
```

标准错误写入文件。

------------------------------------------------------------------------

## 5. diff 比较

命令：

``` bash
diff -wB actual.out expected.out
```

参数：

``` text
-w
忽略空白字符差异

-B
忽略空白行
```

返回值：

``` text
0:
文件相同

1:
文件不同

>1:
diff 执行错误
```

------------------------------------------------------------------------

## 6. /dev/null

Linux 黑洞文件。

写入内容会被丢弃。

例如：

``` bash
command > /dev/null
```

隐藏标准输出。

------------------------------------------------------------------------

# MiniJudge v0.1

## 7. compile()

作用：

-   调用 g++ 编译用户代码
-   生成可执行文件
-   保存编译错误日志
-   返回编译结果

接口：

``` cpp
bool compile(
    const string& code,
    const string& exe
);
```

实际命令：

``` bash
g++ source.cpp -std=c++17 -O2 -o program 2> tmp/compile.log
```

常见坑：

不能通过：

``` text
可执行文件是否存在
```

判断编译成功。

原因：

旧文件可能残留。

------------------------------------------------------------------------

## 8. run()

作用：

运行用户程序。

流程：

``` text
输入文件
 ↓
stdin
 ↓
用户程序
 ↓
stdout
 ↓
输出文件
```

命令：

``` bash
./program < test.in > actual.out
```

当前：

`system()` 非 0 只能认为运行失败。

无法精确判断：

-   RE
-   TLE

------------------------------------------------------------------------

## 9. compare()

作用：

比较：

``` text
实际输出
vs
标准答案
```

使用：

``` bash
diff
```

当前流程：

``` text
编译
 ↓
运行
 ↓
比较
 ↓
AC / WA / CE
```

------------------------------------------------------------------------

# Git

## 10. Git 基本流程

三个区域：

``` text
工作区
 ↓ git add
暂存区
 ↓ git commit
本地仓库
 ↓ git push
远程仓库
```

------------------------------------------------------------------------

## 11. git init

作用：

初始化 Git 仓库。

生成：

``` text
.git/
```

保存版本信息。

------------------------------------------------------------------------

## 12. .gitignore

作用：

告诉 Git 哪些文件不要提交。

例如：

``` gitignore
tmp/*
!tmp/.gitkeep
```

------------------------------------------------------------------------

## 13. git commit

作用：

保存一次版本。

推荐格式：

``` text
type: description
```

例如：

``` bash
git commit -m "feat: implement basic judge flow"
```

------------------------------------------------------------------------

# C++ 多文件项目

## 14. src 与 include

工程常见结构：

``` text
src/
    .cpp 文件

include/
    .h 文件
```

含义：

``` text
src
source code

include
header files
```

------------------------------------------------------------------------

## 15. 头文件与源文件

`.h`

负责：

声明接口。

`.cpp`

负责：

实现功能。

------------------------------------------------------------------------

## 16. #pragma once

作用：

防止头文件重复包含。

写在头文件顶部：

``` cpp
#pragma once
```

------------------------------------------------------------------------

## 17. 多文件编译

例如：

``` bash
g++ src/main.cpp src/Compiler.cpp \
-Iinclude \
-std=c++17 \
-Wall \
-Wextra \
-o tmp/minijudge
```

说明：

多个 `.cpp` 一起参与编译和链接。

------------------------------------------------------------------------

# C++ 命名空间

## 18. std::

`std` 是 C++ 标准库命名空间。

例如：

``` cpp
std::string
std::cout
std::vector
```

------------------------------------------------------------------------

## 19. using namespace std

可以省略：

``` cpp
std::
```

但是工程中：

-   `.cpp` 可以使用
-   `.h` 不建议使用

------------------------------------------------------------------------

# GCC 编译参数

## 20. -Wall

开启常见编译警告。

------------------------------------------------------------------------

## 21. -Wextra

开启更多额外警告。

通常：

``` bash
-Wall -Wextra
```

一起使用。

------------------------------------------------------------------------

# GitHub

## 22. remote

添加远程仓库：

``` bash
git remote add origin 仓库地址
```

查看：

``` bash
git remote -v
```

------------------------------------------------------------------------

## 23. git push

第一次：

``` bash
git push -u origin main
```

以后：

``` bash
git push
```

------------------------------------------------------------------------

## 24. 查看历史

``` bash
git log --oneline
```

查看提交：

``` bash
git show commit_id
```

查看统计：

``` bash
git show --stat
```

------------------------------------------------------------------------

# MiniJudge 当前状态

## v0.1

基础评测流程：

``` text
用户源码
 ↓
compile()
 ↓
生成程序
 ↓
run()
 ↓
实际输出
 ↓
compare()
 ↓
AC / WA / CE
```

------------------------------------------------------------------------

## v0.2

模块拆分完成：

``` text
include/
    Compiler.h
    Runner.h
    Checker.h

src/
    main.cpp
    Compiler.cpp
    Runner.cpp
    Checker.cpp
```

职责：

``` text
Compiler.cpp
编译用户程序

Runner.cpp
运行用户程序

Checker.cpp
比较输出

main.cpp
控制评测流程
```

------------------------------------------------------------------------

------------------------------------------------------------------------

# CMake

## 25. CMakeLists.txt

作用：

统一管理项目的编译和链接。

当前目标：

``` cmake
add_executable(minijudge
    src/main.cpp
    src/Compiler.cpp
    src/Runner.cpp
    src/Checker.cpp
    src/TestCaseFinder.cpp
)
```

头文件目录：

``` cmake
target_include_directories(minijudge PRIVATE include)
```

这样源码中可以直接写：

``` cpp
#include "Compiler.h"
```

------------------------------------------------------------------------

## 26. CMake 构建命令

配置项目：

``` bash
cmake -S . -B build
```

编译项目：

``` bash
cmake --build build
```

含义：

``` text
-S .
源码目录是当前目录

-B build
构建文件放入 build/

cmake --build build
执行实际编译和链接
```

常见坑：

新增 `.cpp` 后必须加入 `CMakeLists.txt`，否则可能出现：

``` text
undefined reference
```

------------------------------------------------------------------------

# 命令行参数

## 27. argc 与 argv

入口：

``` cpp
int main(int argc, char* argv[])
```

运行：

``` bash
./build/minijudge examples/accepted.cpp
```

对应：

``` text
argc == 2
argv[0] == "./build/minijudge"
argv[1] == "examples/accepted.cpp"
```

说明：

``` text
argc
参数数量，包含程序启动路径

argv[0]
程序启动路径

argv[1]
第一个用户参数
```

使用前检查：

``` cpp
if (argc < 2) {
    std::cerr << "Usage: "
              << argv[0]
              << " <source_path>\n";
    return 1;
}
```

读取源码路径：

``` cpp
std::string code = argv[1];
```

常见坑：

检查 `argc` 前不能访问 `argv[1]`。

------------------------------------------------------------------------

# C++ 文件系统

## 28. std::filesystem

需要：

``` cpp
#include <filesystem>
```

检查路径：

``` cpp
std::filesystem::exists(path);
std::filesystem::is_directory(path);
```

区别：

``` text
exists()
路径是否存在

is_directory()
路径是否是目录
```

常见坑：

路径存在，不代表它一定是目录。

------------------------------------------------------------------------

## 29. 遍历目录

``` cpp
for (const auto& entry :
     std::filesystem::directory_iterator(testDir)) {
    if (!entry.is_regular_file()) {
        continue;
    }
}
```

说明：

``` text
directory_iterator
依次访问目录项

is_regular_file()
判断是否为普通文件
```

目录遍历顺序没有保证。

------------------------------------------------------------------------

## 30. path 文件名操作

``` cpp
std::filesystem::path path = entry.path();

std::string filename =
    path.filename().string();

std::string pre =
    path.stem().string();

std::string suf =
    path.extension().string();
```

例如：

``` text
文件名       stem       extension
1.in         1          .in
24#2.out     24#2       .out
.in          .in        空字符串
```

注意：

`.in` 是 Linux 隐藏文件名，不是：

``` text
空主名 + .in 扩展名
```

因此需要单独判断完整文件名：

``` cpp
if (filename == ".in" ||
    filename == ".out") {
    // 非法空测试名
}
```

------------------------------------------------------------------------

## 31. filesystem 路径拼接

推荐：

``` cpp
std::filesystem::path input =
    std::filesystem::path(testDir) /
    (name + ".in");
```

需要传给 `string` 参数时：

``` cpp
input.string()
```

常见坑：

手动拼接：

``` cpp
testDir + name + ".in"
```

会依赖 `testDir` 末尾是否有 `/`。

------------------------------------------------------------------------

# 测试点自动发现

## 32. 测试点配对规则

当前规则：

``` text
同名的 <name>.in 和 <name>.out
组成一个测试点
```

例如：

``` text
1.in       1.out
24#2.in    24#2.out
sample.in  sample.out
```

测试名不要求是连续整数。

------------------------------------------------------------------------

## 33. 使用 set 保存测试名

``` cpp
std::set<std::string> inputNames;
std::set<std::string> outputNames;
```

发现输入文件：

``` cpp
inputNames.insert(pre);
```

检查是否有对应输出：

``` cpp
if (outputNames.count(name) == 0) {
    // 缺少标准答案
}
```

双向检查：

``` text
遍历 inputNames
检查对应 .out

遍历 outputNames
检查对应 .in
```

生成测试列表：

``` cpp
testNames.assign(
    inputNames.begin(),
    inputNames.end()
);
```

`set<string>` 使用字符串字典序：

``` text
1 → 10 → 2 → 24#2
```

常见坑：

统计目录文件数量再除以 2，无法识别缺失配对和无关文件。

------------------------------------------------------------------------

## 34. 测试名白名单

当前允许：

``` text
A-Z
a-z
0-9
_
-
#
.
```

判断：

``` cpp
bool valid =
    ('a' <= ch && ch <= 'z') ||
    ('A' <= ch && ch <= 'Z') ||
    ('0' <= ch && ch <= '9') ||
    ch == '_' ||
    ch == '-' ||
    ch == '#' ||
    ch == '.';
```

原因：

当前通过 `system()` 拼接 Shell 命令，需要避免空格和 Shell 特殊字符改变命令含义。

结论：

白名单比不断补充危险字符黑名单更可靠。

常见坑：

应先确认扩展名是 `.in` 或 `.out`，再检查测试名；否则无关文件也会被误判。

------------------------------------------------------------------------

## 35. findTestCases()

接口：

``` cpp
bool findTestCases(
    const std::string& testDir,
    std::vector<std::string>& testNames,
    std::string& errMessage
);
```

参数：

``` text
testDir
输入：测试目录

testNames
输出：测试点名称

errMessage
输出：失败原因

返回值
是否成功
```

函数开始时：

``` cpp
testNames.clear();
errMessage.clear();
```

成功时：

``` cpp
testNames.assign(
    inputNames.begin(),
    inputNames.end()
);
return true;
```

失败时：

``` cpp
errMessage = "...";
return false;
```

常见坑：

函数返回失败后，调用方不能继续使用 `testNames`。

------------------------------------------------------------------------

# Git 补充

## 36. 修改最近一次提交

如果最近一次提交尚未推送，并且要把遗漏内容补进同一个提交：

``` bash
git add 文件
git commit --amend --no-edit
```

作用：

``` text
amend
修改最近一次提交

--no-edit
保留原提交信息
```

提交哈希会改变。

------------------------------------------------------------------------

## 37. 撤回提交但保留代码

``` bash
git reset --soft HEAD~1
```

结果：

``` text
最近一次提交被撤回
代码修改不会丢失
修改仍保留在暂存区
```

不要随意使用：

``` bash
git reset --hard HEAD~1
```

因为它会删除工作区修改。

------------------------------------------------------------------------

# 工程设计结论

## 38. 不要过早抽象

只有数据需要：

``` text
保存
传递
统计
统一处理
```

时，才需要额外类型或转换函数。

当前评测结果判断后立即输出，因此暂时不需要：

``` cpp
enum class JudgeResult
```

常见坑：

为了使用新语法，增加没有实际收益的变量和函数。

------------------------------------------------------------------------

# MiniJudge 当前状态

## v0.1

基础评测流程完成：

``` text
compile
→ run
→ compare
→ AC / WA / CE / Run failed
```

------------------------------------------------------------------------

## v0.2

基础模块拆分完成：

``` text
Compiler
Runner
Checker
main
```

------------------------------------------------------------------------

## v0.3

CMake 构建完成：

``` bash
cmake -S . -B build
cmake --build build
```

------------------------------------------------------------------------

## v0.4

命令行源码路径完成：

``` bash
./build/minijudge examples/accepted.cpp
```

------------------------------------------------------------------------

## v0.5

测试点自动发现完成：

``` text
TestCaseFinder
扫描 tests/
校验 .in / .out 配对
返回测试点名称列表
```

当前项目结构：

``` text
include/
    Compiler.h
    Runner.h
    Checker.h
    TestCaseFinder.h

src/
    main.cpp
    Compiler.cpp
    Runner.cpp
    Checker.cpp
    TestCaseFinder.cpp
```

------------------------------------------------------------------------

# 当前尚未完成

``` text
每个测试点运行时间统计
TLE
精确区分 RE
文件系统异常处理
Shell 参数完整转义
支持带空格的路径
修复必须从项目根目录运行的问题
fork / exec / dup2 / waitpid
内存和其他资源限制
结构化评测报告
```

