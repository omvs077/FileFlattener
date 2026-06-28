cmake_minimum_required(VERSION 3.21)
project(FileFlattener CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(core)

add_executable(FileFlattenerCLI
    app/main_cli.cpp
)

target_link_libraries(FileFlattenerCLI PRIVATE FileFlattenerCore)
