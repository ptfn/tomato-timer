cmake_minimum_required(VERSION 3.22.1)

project(tomato-timer)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(main src/clock.c)
target_link_libraries(main ${CURSES_LIBRARIES})

