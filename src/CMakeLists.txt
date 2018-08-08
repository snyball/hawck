#time g++ -Wall -Wextra -std=c++17 -fsanitize=address -O2 UDevice.cpp hawck.cpp LuaUtils.cpp Keyboard.cpp -fsanitize=address -llua5.3 -o hawck

cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 17)
add_definitions(
    #-masm=intel
    -Wall
    -Wextra
    -std=c++17
    # -fsanitize=address
    -O1 
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
    CSV.cpp
)

target_link_libraries(hawck.x64
    -llua5.3
    -lnotify
    -ldbus-glib-1 -ldbus-1 -lgobject-2.0 -lglib-2.0
    -fsanitize=address
)

## ======
## Tests
## ======

add_executable(tests.x64
    tests/CSV-tests/CSV-tests.cpp
    tests/tests-main.cpp
    CSV.cpp
)

# target_link_libraries(test-CSV.x64
#     -fsanitize=address
# )
