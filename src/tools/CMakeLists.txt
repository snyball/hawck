cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 17)
add_definitions(
    -Wall
    -Wextra
    -std=c++17
    -Os 
)
include_directories("..")

add_executable(lsinput
    lsinput.cpp
)

target_link_libraries(lsinput)
