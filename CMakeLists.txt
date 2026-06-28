cmake_minimum_required(VERSION 3.21)
project(FileFlattener CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ZLIB REQUIRED)
find_package(minizip CONFIG REQUIRED)
find_path(PICOSHA2_INCLUDE_DIRS "picosha2.h")

add_executable(FileFlattener
    src/main.cpp
    src/FileScanner.cpp
    src/Deduplicator.cpp
    src/Renamer.cpp
)

target_include_directories(FileFlattener PRIVATE ${PICOSHA2_INCLUDE_DIRS})

target_link_libraries(FileFlattener PRIVATE
    ZLIB::ZLIB
    $<IF:$<TARGET_EXISTS:MINIZIP::minizip>,MINIZIP::minizip,MINIZIP::minizipstatic>
)
