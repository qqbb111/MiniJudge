项目框架：各个文件夹内容 / 功能：
/examples：存放用户代码
/tests：存放测试数据以及预期输出
/tmp：存放临时生成变量————用户代码编译后的可执行程序、用户代码运行测试数据后生成的实际输出

main.cpp：负责完成用户代码的读取、编译、运行，将运行得到的结果与预期输出对比；输出评测结果

基础流程：
Linux语言：
g++ examples/accepted.cpp -std=c++17 -O2 -o tmp/user_program 2> tmp/compile.log #g++用C++17标准编译用户代码，开O2优化，生成可执行程序到tmp文件夹。并把标准错误（2：stderr）重定向到文件里
./tmp/user_program < tests/1.in > tmp/actual.out #将测试数据写入程序，输出结果存放在tmp文件夹里
diff -wB tmp/actual.out tests/1.out #比较结果 -w：忽略全部的空白（Tab / Space）字符；-B：不检查空白行
