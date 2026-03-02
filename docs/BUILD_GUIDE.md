# Build Guide - Banking System Week 1

This guide provides step-by-step instructions for building and running the project.

## System Requirements

### Minimum Requirements
- **CPU**: Dual-core processor (quad-core recommended)
- **RAM**: 2GB minimum (4GB recommended)
- **Disk Space**: 500MB free space
- **OS**: Linux (Ubuntu 18.04+, Debian 10+) or WSL2 on Windows

### Software Requirements
- g++ 7.0 or higher
- CMake 3.10 or higher
- make
- pthread library (usually included)

## Installation Guide

### Ubuntu/Debian/WSL

```bash
# Update package list
sudo apt-get update

# Install all required packages
sudo apt-get install -y build-essential cmake g++ git

# Verify installations
g++ --version      # Should show 7.0 or higher
cmake --version    # Should show 3.10 or higher
make --version     # Should show any version
```

### Fedora/RHEL/CentOS

```bash
# Install development tools
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake gcc-c++

# Verify installations
g++ --version
cmake --version
```

### Arch Linux

```bash
# Install build tools
sudo pacman -S base-devel cmake

# Verify installations
g++ --version
cmake --version
```

## Building the Project

### Method 1: Standard Build (Recommended)

```bash
# Step 1: Navigate to project directory
cd banking-system-week1

# Step 2: Create build directory
mkdir build
cd build

# Step 3: Generate build files with CMake
cmake ..

# Expected output:
# -- The C compiler identification is GNU X.X.X
# -- The CXX compiler identification is GNU X.X.X
# -- Detecting C compiler ABI info
# -- Detecting C compiler ABI info - done
# -- Check for working C compiler: /usr/bin/cc - skipped
# -- Detecting C compile features
# -- Detecting C compile features - done
# ...
# -- Configuring done
# -- Generating done
# -- Build files have been written to: /path/to/build

# Step 4: Compile the project
make

# Expected output:
# Scanning dependencies of target banking_system
# [ 16%] Building CXX object CMakeFiles/banking_system.dir/src/Account.cpp.o
# [ 33%] Building CXX object CMakeFiles/banking_system.dir/src/Transaction.cpp.o
# [ 50%] Building CXX object CMakeFiles/banking_system.dir/src/ThreadPool.cpp.o
# [ 66%] Building CXX object CMakeFiles/banking_system.dir/src/main.cpp.o
# [100%] Linking CXX executable banking_system
# [100%] Built target banking_system

# Step 5: Run the program
./banking_system
```

### Method 2: Quick Build Script

Create a file named `build.sh` in the project root:

```bash
#!/bin/bash

echo "Building Banking System..."

# Clean previous build
rm -rf build
mkdir build
cd build

# Configure and build
cmake .. && make

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Build successful!"
    echo "Run the program with: ./build/banking_system"
else
    echo ""
    echo "❌ Build failed. Check errors above."
    exit 1
fi
```

Make it executable and run:

```bash
chmod +x build.sh
./build.sh
```

### Method 3: CMake with Verbose Output

To see detailed compilation commands:

```bash
mkdir build && cd build
cmake ..
make VERBOSE=1
```

## Running the Program

### Basic Run

```bash
cd build
./banking_system
```

### Run with Output Redirection

Save output to a file for analysis:

```bash
./banking_system > output.log 2>&1
```

### Run with Valgrind (Memory Leak Detection)

If you have valgrind installed:

```bash
valgrind --leak-check=full ./banking_system
```

## Troubleshooting

### Problem: "cmake: command not found"

**Solution:**
```bash
sudo apt-get install cmake
```

### Problem: "g++: command not found"

**Solution:**
```bash
sudo apt-get install g++
```

### Problem: "pthread library not found"

**Solution:**
```bash
sudo apt-get install libpthread-stubs0-dev
```

### Problem: Compilation errors about C++17

**Solution:**
Your g++ version is too old. Update it:

```bash
sudo apt-get update
sudo apt-get install g++-9
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 90
```

### Problem: "No such file or directory" errors

**Solution:**
Verify project structure:

```bash
cd banking-system-week1
ls -R

# Should show:
# CMakeLists.txt  README.md  src/
# 
# ./src:
# Account.cpp  Account.h  main.cpp  Transaction.cpp  Transaction.h  ThreadPool.cpp  ThreadPool.h
```

### Problem: Build succeeds but program crashes

**Solution:**
1. Check for stack overflow (increase stack size):
   ```bash
   ulimit -s unlimited
   ./banking_system
   ```

2. Run with debugger:
   ```bash
   gdb ./banking_system
   (gdb) run
   ```

### Problem: Program hangs (should not happen)

**Solution:**
If the program hangs (which indicates a deadlock bug):

1. Press `Ctrl+C` to stop
2. Run with debugger to see where it's stuck:
   ```bash
   gdb ./banking_system
   (gdb) run
   # When it hangs, press Ctrl+C
   (gdb) thread apply all bt  # Show all thread backtraces
   ```

## Clean Build

To start fresh:

```bash
# Remove build directory
rm -rf build

# Rebuild
mkdir build && cd build
cmake ..
make
```

## Performance Optimization

### Release Build (Faster Execution)

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

### Debug Build (Better Error Messages)

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## Advanced Options

### Change Number of Threads

Edit `src/main.cpp`, line where ThreadPool is created:

```cpp
// Change from:
ThreadPool pool(4);

// To (use all CPU cores):
ThreadPool pool(std::thread::hardware_concurrency());
```

### Enable Compiler Warnings

Already enabled in CMakeLists.txt:
```cmake
target_compile_options(banking_system PRIVATE -Wall -Wextra)
```

To add more warnings:
```cmake
target_compile_options(banking_system PRIVATE -Wall -Wextra -Wpedantic -Werror)
```

### Static Analysis with Clang-Tidy

```bash
# Install clang-tidy
sudo apt-get install clang-tidy

# Run analysis
cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
clang-tidy ../src/*.cpp
```

## Testing the Build

### Quick Smoke Test

After building, verify the program runs:

```bash
cd build
timeout 30s ./banking_system < /dev/null
```

If it completes within 30 seconds, build is successful.

### Verify OS Concepts

Run the program and check:

1. ✅ Multiple threads are created (4 workers)
2. ✅ Deadlock prevention demo completes
3. ✅ Mutex demo shows correct final balance
4. ✅ Thread pool distributes work
5. ✅ Program exits cleanly

## IDE Integration

### VSCode

Create `.vscode/tasks.json`:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build Banking System",
            "type": "shell",
            "command": "mkdir -p build && cd build && cmake .. && make",
            "group": {
                "kind": "build",
                "isDefault": true
            }
        }
    ]
}
```

Press `Ctrl+Shift+B` to build.

### CLion

CLion automatically detects CMakeLists.txt. Just open the project folder.

## Continuous Integration

### GitHub Actions Example

Create `.github/workflows/build.yml`:

```yaml
name: Build and Test

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Install dependencies
      run: sudo apt-get install -y build-essential cmake g++
    
    - name: Build
      run: |
        mkdir build
        cd build
        cmake ..
        make
    
    - name: Test
      run: |
        cd build
        timeout 30s ./banking_system < /dev/null || exit 0
```

## Platform-Specific Notes

### Windows (WSL)

Use WSL2 for best performance:

```bash
# Install WSL2 (PowerShell as Administrator)
wsl --install

# Inside WSL
sudo apt-get update
sudo apt-get install build-essential cmake g++
```

### macOS

Install Xcode Command Line Tools:

```bash
xcode-select --install

# Install CMake
brew install cmake

# Build normally
mkdir build && cd build
cmake ..
make
```

### Docker

Create `Dockerfile`:

```dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN mkdir build && cd build && cmake .. && make

CMD ["./build/banking_system"]
```

Build and run:

```bash
docker build -t banking-system .
docker run -it banking-system
```

## Next Steps

After successful build:

1. ✅ Run all three demo scenarios
2. ✅ Review the code comments
3. ✅ Experiment with different thread counts
4. ✅ Read CONCEPTS.md to understand OS concepts
5. ✅ Prepare for Week 2 (database integration)

## Getting Help

If you encounter issues:

1. Check this BUILD_GUIDE.md
2. Review error messages carefully
3. Search the error online
4. Check project structure is correct
5. Try clean build (`rm -rf build && mkdir build && cd build && cmake .. && make`)

---

*Last updated: Week 1*
*For OS concepts explanation, see CONCEPTS.md*
