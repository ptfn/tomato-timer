cmake_minimum_required(VERSION 3.22.1)

project(tomato-timer)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "~/.local/bin")

add_executable(tomato src/clock.c)
target_link_libraries(tomato ${CURSES_LIBRARIES})

