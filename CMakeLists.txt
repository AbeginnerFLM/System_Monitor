cmake_minimum_required(VERSION 3.10)

# 项目名称和版本
project(SystemMonitor VERSION 1.0 LANGUAGES CXX)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# 设置 Debug 模式的编译选项
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# 自动生成 compile_commands.json（关键！）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 源文件
set(SOURCES
    src/main.cpp
    src/Collectors.cpp
)

# 生成可执行文件
add_executable(system_monitor ${SOURCES})
