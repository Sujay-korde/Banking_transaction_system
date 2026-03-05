cmake_minimum_required(VERSION 3.10)
project(BankingSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

execute_process(
    COMMAND mysql_config --cflags
    OUTPUT_VARIABLE MYSQL_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND mysql_config --libs
    OUTPUT_VARIABLE MYSQL_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MYSQL_CFLAGS}")

add_executable(banking_system
    src/main.cpp
    src/Account.cpp
    src/Transaction.cpp
    src/ThreadPool.cpp
    src/ConnectionPool.cpp
    src/DatabaseManager.cpp
    src/BoundedQueue.cpp
)

target_include_directories(banking_system PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(banking_system PRIVATE Threads::Threads ${MYSQL_LIBS})
