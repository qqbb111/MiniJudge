cmake_minimum_required(VERSION 3.12)
project(MiniJudge)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(
    minijudge
    ${SOURCES}
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
