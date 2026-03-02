# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name
project(BankingSystem)

# Use C++17 standard (modern C++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find threading library (for pthread)
find_package(Threads REQUIRED)

# Create executable from all source files
add_executable(banking_system
    src/main.cpp
    src/Account.cpp
    src/Transaction.cpp
    src/ThreadPool.cpp
)

# Link pthread library for threading support
target_link_libraries(banking_system PRIVATE Threads::Threads)

# Add include directories
target_include_directories(banking_system PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Enable compiler warnings (helps catch errors)
target_compile_options(banking_system PRIVATE -Wall -Wextra)
