#time g++ -Wall -Wextra -std=c++17 -fsanitize=address -O2 UDevice.cpp hawck.cpp LuaUtils.cpp Keyboard.cpp -fsanitize=address -llua5.3 -o hawck

cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 17)
add_definitions(
    #-masm=intel
    -Wall
    -Wextra
    -std=c++17
    #-fsanitize=address
    -Os 
    -g
    -rdynamic
    -pthread
)

add_executable(hawck-macrod
    RemoteUDevice.cpp
    Daemon.cpp
    MacroDaemon.cpp
    LuaUtils.cpp
    Keyboard.cpp
    FSWatcher.cpp
    hawck-macrod.cpp
)

target_link_libraries(hawck-macrod
    -llua5.3
    -lnotify
    -ldbus-glib-1 -ldbus-1 -lgobject-2.0 -lglib-2.0
    -lpthread
    #-fsanitize=address
)

add_executable(hawck-inputd
    UDevice.cpp
    Daemon.cpp
    KBDDaemon.cpp
    Keyboard.cpp
    FSWatcher.cpp
    CSV.cpp
    hawck-inputd.cpp
)

target_link_libraries(hawck-inputd
    -lpthread
    #-fsanitize=address
)

## ======
## Tests
## ======

add_executable(tests.x64
    tests/CSV-tests/CSV-tests.cpp
    tests/FSWatcher-tests/FSWatcher-tests.cpp
    tests/tests-main.cpp
    FSWatcher.cpp
    CSV.cpp
)

target_link_libraries(tests.x64
    #-fsanitize=address
    -lpthread
)
