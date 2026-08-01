# MiniJudge 学习笔记

## 1. `const T&` 传参

结论：只读的 `string`、`vector` 等对象通常使用 `const T&`，避免复制，也防止函数误修改。

```cpp
bool run(const string& path);
```

- `T`：复制一份
- `T&`：不复制，可以修改原对象
- `const T&`：不复制，不能修改原对象

---

## 2. `c_str()`

结论：把 `std::string` 转成 C 接口需要的 `const char*`。

```cpp
string cmd = "ls";
system(cmd.c_str());
```

常见坑：原 `string` 修改或销毁后，之前取得的指针可能失效。

---

## 3. `system()`

结论：让 shell 执行一条命令。

```cpp
int res = system(cmd.c_str());
return !res;
```

当前阶段：返回 `0` 看作成功，非 `0` 看作失败。

常见坑：非 `0` 不一定都是同一种错误，当前只能笼统表示命令失败。

---

## 4. Linux 重定向

```bash
< input.txt       # 标准输入来自文件
> output.txt      # 覆盖写入标准输出
2> error.log      # 覆盖写入标准错误
2>> error.log     # 追加标准错误
```

文件描述符：

```text
0：标准输入
1：标准输出
2：标准错误
```

---

## 5. 退出状态与 `$?`

结论：`$?` 保存上一条命令的退出状态，`echo $?` 可以显示它。

```bash
diff -wB actual.out expected.out
echo $?
```

`diff` 常见返回值：

- `0`：文件相同
- `1`：文件不同
- `>1`：`diff` 执行出错

---

## 6. `compile()`

```cpp
bool compile(const string& code, const string& exe);
```

作用：

- 调用 `g++` 编译源码
- 生成指定可执行文件
- 把错误写入 `tmp/compile.log`
- 成功返回 `true`，失败返回 `false`

实际命令：

```bash
g++ source.cpp -std=c++17 -O2 -o program 2> tmp/compile.log
```

常见坑：

- 旧的可执行文件可能仍然存在，不能靠文件是否存在判断本次编译是否成功
- 当前字符串拼接方式不支持带空格的路径

---

## 7. `run()`

```cpp
bool run(const string& exe,
         const string& input,
         const string& output);
```

作用：运行用户程序，把测试输入重定向给它，并保存实际输出。

```bash
./program < test.in > actual.out
```

常见坑：

- 当前目录下的可执行文件通常需要写 `./`
- `system()` 返回非零目前只能称为运行失败，不能精确断定一定是 RE

---

## 8. `compare()`

```cpp
bool compare(const string& actual,
             const string& expected);
```

作用：使用 `diff` 比较实际输出与标准答案。

```bash
diff -wB actual.out expected.out
```

- `-w`：忽略空白字符差异
- `-B`：忽略整行为空白的行
- 返回 `0`：判为 AC
- 返回非 `0`：当前判为未通过

常见坑：`diff` 默认会把不同内容打印到终端。

可隐藏输出：

```bash
diff -wB actual.out expected.out > /dev/null 2>&1
```

- `/dev/null`：丢弃写入其中的内容
- `2>&1`：让标准错误和标准输出写到同一位置

---

## 9. 当前已跑通的流程

```text
编译用户代码
→ 循环读取测试点
→ 运行用户程序
→ 保存实际输出
→ 与标准答案比较
→ 输出 CE / Run failed / AC / WA
```

当前仍是第一阶段版本，使用 `std::system()`，尚未精确实现 RE、TLE 和运行时间统计。