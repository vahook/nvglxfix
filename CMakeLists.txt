cmake_minimum_required(VERSION 3.20)
project(nvglxfix)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS OFF)

find_package(X11 REQUIRED xcb xcb_sync)
add_library(nvglxfix SHARED main.c)
target_link_libraries(nvglxfix PRIVATE X11::xcb X11::xcb_sync)

find_package(Threads REQUIRED)
add_executable(xspam xspam.c)
target_link_libraries(xspam PRIVATE X11::xcb Threads::Threads)