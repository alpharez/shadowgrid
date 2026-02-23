cmake_minimum_required(VERSION 3.15)
project(shadowgrid C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Curses REQUIRED)

file(GLOB SOURCES "src/*.c")

add_executable(shadowgrid ${SOURCES})

target_include_directories(shadowgrid PRIVATE src ${CURSES_INCLUDE_DIRS})
target_link_libraries(shadowgrid ${CURSES_LIBRARIES})

target_compile_options(shadowgrid PRIVATE -Wall -Wextra -pedantic)
