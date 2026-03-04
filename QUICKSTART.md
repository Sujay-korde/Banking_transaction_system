# QUICK START GUIDE

Get up and running in 5 minutes!

## Step 1: Install Dependencies (2 minutes)

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake g++
```

## Step 2: Build Project (1 minute)

### Option A: Using build script (Easiest)

```bash
chmod +x build.sh
./build.sh
```

### Option B: Manual build

```bash
mkdir build && cd build
cmake ..
make
```

## Step 3: Run Program (2 minutes)

```bash
cd build
./banking_system
```

## What You'll See

The program runs 3 interactive demos:

1. **Demo 1: Deadlock Prevention**
   - Shows two simultaneous transfers
   - Press Enter to continue

2. **Demo 2: Critical Section (Mutex)**
   - Shows 10 concurrent deposits
   - Press Enter to continue

3. **Demo 3: Thread Pool Scheduling**
   - Shows 20 mixed transactions
   - Automatically completes

## Troubleshooting

### "cmake: command not found"
```bash
sudo apt-get install cmake
```

### "g++: command not found"
```bash
sudo apt-get install g++
```

### Build errors
```bash
# Clean and rebuild
rm -rf build
mkdir build && cd build
cmake ..
make
```

## Next Steps

After running successfully:

1. Read `README.md` for full documentation
2. Read `docs/CONCEPTS.md` to understand OS concepts
3. Review code in `src/` folder
4. Experiment with changing thread count

## File Structure

```
banking-system-week1/
├── build.sh              ← Run this to build
├── CMakeLists.txt        ← Build configuration
├── README.md             ← Full documentation
├── src/
│   ├── main.cpp          ← Start reading here
│   ├── Account.h/cpp     ← Account with mutex
│   ├── Transaction.h/cpp ← Transaction with deadlock prevention
│   └── ThreadPool.h/cpp  ← Thread pool scheduler
└── docs/
    ├── BUILD_GUIDE.md    ← Detailed build instructions
    └── CONCEPTS.md       ← OS concepts explained
```

## Quick Commands

```bash
# Build
./build.sh

# Run
./build/banking_system

# Clean and rebuild
rm -rf build && ./build.sh

# View code
cat src/main.cpp
cat src/Account.cpp
cat src/Transaction.cpp
cat src/ThreadPool.cpp
```

## Expected Runtime

- Demo 1: ~3 seconds
- Demo 2: ~2 seconds  
- Demo 3: ~5 seconds
- Total: ~10 seconds (plus time to read and press Enter)

## Success Indicators

✅ Program creates 5 accounts  
✅ Thread pool starts with 4 workers  
✅ Demo 1 completes both transfers  
✅ Demo 2 shows correct final balance  
✅ Demo 3 processes all 20 transactions  
✅ Program exits cleanly  

If all above happen, congratulations! Your build is successful.

---

**Need help?** See `docs/BUILD_GUIDE.md` for detailed troubleshooting.
