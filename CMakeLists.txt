cmake_minimum_required(VERSION 3.21)
project(FileFlattener CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

add_subdirectory(core)

add_executable(FileFlattenerCLI
    app/main_cli.cpp
)
target_link_libraries(FileFlattenerCLI PRIVATE FileFlattenerCore)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(FileFlattenerApp
    app/main_gui.cpp
    app/MainWindow.cpp
    app/include/MainWindow.h
)
target_include_directories(FileFlattenerApp PRIVATE app/include)
target_link_libraries(FileFlattenerApp PRIVATE FileFlattenerCore Qt6::Widgets)

set_target_properties(FileFlattenerApp PROPERTIES
    WIN32_EXECUTABLE TRUE
)

find_program(WINDEPLOYQT_EXE windeployqt HINTS "C:/Qt/6.11.1/msvc2022_64/bin")
if(WINDEPLOYQT_EXE)
    add_custom_command(TARGET FileFlattenerApp POST_BUILD
        COMMAND ${WINDEPLOYQT_EXE} $<TARGET_FILE:FileFlattenerApp>
        COMMENT "Deploying Qt dependencies"
    )
endif()

