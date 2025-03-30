cmake_minimum_required(VERSION 3.20)
project(nvglxfix)

find_package(X11 REQUIRED xcb xcb_sync)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS OFF)

add_library(nvglxfix SHARED main.c)

target_link_libraries(nvglxfix X11::xcb X11::xcb_sync)