cmake_minimum_required(VERSION 3.10)
project(ChatServer)

set(CMAKE_BUILD_TYPE "Debug")

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 编译选项
add_compile_options(-Wall -O2)

# 头文件目录
set(SRC_DIR ${CMAKE_SOURCE_DIR}/testmuduo)
set(INC_DIR ${CMAKE_SOURCE_DIR}/include)
set(TEST_DIR ${CMAKE_SOURCE_DIR}/tests)

# 源文件列表：包含 testmuduo 下的所有 .cpp 文件
file(GLOB SOURCES "${SRC_DIR}/*.cpp")

# 添加头文件目录
include_directories(${INC_DIR})

# 创建可执行文件（假设 server 是主程序）
add_executable(muduo_server
    ${SOURCES}
)

# 链接库（注意顺序和目标名称要与 add_executable 匹配）
target_link_libraries(muduo_server
    PRIVATE
    muduo_net
    muduo_base
    pthread
)
