cmake_minimum_required(VERSION 3.10)
project(MiniJudge)

set(CMAKE_CXX_STANDARD 17)

add_executable(
    minijudge
    src/main.cpp
    src/Compiler.cpp
    src/Runner.cpp
    src/Checker.cpp
    src/TestCasesFinder.cpp
)

target_include_directories(
    minijudge 
    PRIVATE 
    include
)

target_compile_options(
    minijudge 
    PRIVATE 
    -Wall 
    -Wextra 
    -Wpedantic
)
