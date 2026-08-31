cmake_minimum_required(VERSION 3.16)
project(mac_sweep)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)   # Q_OBJECT 자동 처리

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(mac_sweep
    src/main.cpp
    src/utils.cpp
    src/scanner.cpp
    src/gui/main_window.cpp
    include/gui/main_window.h
)

target_include_directories(mac_sweep PRIVATE include)
target_link_libraries(mac_sweep PRIVATE Qt6::Widgets)