cmake_minimum_required(VERSION 3.10)
project(simple_server)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 在文件开头添加 OpenSSL 查找
find_package(OpenSSL REQUIRED)

# 添加头文件路径
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/HttpServer/include
    ${PROJECT_SOURCE_DIR}/WebApps/GomokuServer/include
    /usr/include/mysql-cppconn-8
    /usr/include/mysql
    ${OPENSSL_INCLUDE_DIR}
)

# 查找必要的库
find_library(MYSQLCPPCONN_LIBRARY
    NAMES mysqlcppconn mysqlcppconn8
    PATHS /usr/lib /usr/lib64 /usr/local/lib
)

# 添加所有源文件
file(GLOB_RECURSE HTTP_SERVER_SRC
    "${PROJECT_SOURCE_DIR}/HttpServer/src/*.cpp"
)

file(GLOB_RECURSE GOMOKU_SERVER_SRC
    "${PROJECT_SOURCE_DIR}/WebApps/GomokuServer/src/*.cpp"
)

# 添加主程序文件
set(MAIN_SRC "${PROJECT_SOURCE_DIR}/WebApps/GomokuServer/src/main.cpp")

# 如果主程序文件不存在，创建一个
if(NOT EXISTS ${MAIN_SRC})
    file(WRITE ${MAIN_SRC}
"#include \"../include/GomokuServer.h\"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        GomokuServer server;
        server.start();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << \"Error: \" << e.what() << std::endl;
        return 1;
    }
}
")
endif()

# 添加可执行文件
add_executable(simple_server
    ${MAIN_SRC}
    ${HTTP_SERVER_SRC}
    ${GOMOKU_SERVER_SRC}
)

# 链接必要的库
target_link_libraries(simple_server
    pthread
    muduo_net
    muduo_base
    mysqlcppconn
    mysqlclient
    ssl
    crypto
)

# 打印调试信息
message(STATUS "Include directories:")
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
    message(STATUS "  ${dir}")
endforeach()

message(STATUS "Main source file: ${MAIN_SRC}")
message(STATUS "HTTP Server sources: ${HTTP_SERVER_SRC}")
message(STATUS "Gomoku Server sources: ${GOMOKU_SERVER_SRC}")