#time g++ -Wall -Wextra -std=c++17 -fsanitize=address -O2 UDevice.cpp hawck.cpp LuaUtils.cpp Keyboard.cpp -fsanitize=address -llua5.3 -o hawck

cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 17)
add_definitions(
    -masm=intel
    -Wall
    -Wextra
    -std=c++17
    -fsanitize=address
    -O2 
    -g
)

add_executable(hawck.x86_64
    UDevice.cpp
    RemoteUDevice.cpp
    hawck-s.cpp
    MacroDaemon.cpp
    KBDDaemon.cpp
    LuaUtils.cpp
    Keyboard.cpp
)

target_link_libraries(hawck.x86_64
    -llua5.3
    -fsanitize=address
)
