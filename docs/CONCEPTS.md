# OS Concepts Explained - Banking System

This document explains the Operating System concepts demonstrated in the Banking Transaction Processing System.

## Table of Contents

1. [Threads](#1-threads)
2. [Mutex & Critical Section](#2-mutex--critical-section)
3. [Synchronization](#3-synchronization)
4. [Deadlock Prevention](#4-deadlock-prevention)
5. [Process Scheduling](#5-process-scheduling)
6. [Real-World Applications](#6-real-world-applications)

---

## 1. THREADS

### What is it?

A **thread** is the smallest unit of execution within a process. Multiple threads can run concurrently, sharing the same memory space.

### Why do we need it?

In banking systems, thousands of transactions happen simultaneously. Without threads, we'd have to process them one by one (very slow!).

### How we use it in our project

```cpp
// In ThreadPool.cpp
std::thread worker([this] {
    // This code runs concurrently with other threads
    while (true) {
        // Get task from queue
        // Execute transaction
    }
});
```

**Real-world example:**
When 100 people simultaneously try to withdraw money from ATMs, each withdrawal is processed by a different thread.

### Benefits
- ✅ Parallel processing (faster)
- ✅ Better CPU utilization
- ✅ Responsive system (don't wait for one transaction to finish before starting next)

### Code Location
- `src/ThreadPool.cpp` - Thread creation
- `src/main.cpp` - Thread usage in demos

---

## 2. MUTEX & CRITICAL SECTION

### What is it?

A **mutex** (mutual exclusion) is a lock that ensures only ONE thread can access a resource at a time. A **critical section** is the code protected by the mutex.

### Why do we need it?

**Without mutex - RACE CONDITION:**
```
Thread 1: Read balance = $1000
Thread 2: Read balance = $1000
Thread 1: Add $100, write $1100
Thread 2: Add $50, write $1050  ❌ Lost Thread 1's deposit!
```

**With mutex - CORRECT:**
```
Thread 1: Lock, read $1000, add $100, write $1100, unlock
Thread 2: Wait... Lock, read $1100, add $50, write $1150, unlock ✅
```

### How we use it in our project

```cpp
// In Account.cpp
bool Account::deposit(double amount) {
    std::lock_guard<std::mutex> lock(account_mutex);  // LOCK
    
    // CRITICAL SECTION - Only one thread at a time
    balance += amount;
    
    return true;
    // UNLOCK automatically when 'lock' goes out of scope
}
```

### Real-world example

Your bank account balance is protected by a mutex. Even if 10 people try to access it simultaneously, only one transaction happens at a time.

### Benefits
- ✅ Prevents race conditions
- ✅ Data consistency
- ✅ No lost updates

### Code Location
- `src/Account.cpp` - Mutex in deposit/withdraw
- `src/main.cpp` - Demo 2 shows mutex in action

---

## 3. SYNCHRONIZATION

### What is it?

**Synchronization** is coordinating multiple threads so they work together correctly without conflicts.

### Why do we need it?

Multiple threads accessing shared resources (like bank accounts) need to coordinate to avoid:
- Race conditions
- Deadlocks
- Data corruption

### Synchronization Primitives Used

#### Mutex (std::mutex)
Protects account balance from concurrent access.

#### Condition Variable (std::condition_variable)
Used in thread pool to:
- Wake up sleeping worker threads when tasks arrive
- Make threads wait when queue is empty

```cpp
// In ThreadPool.cpp
condition.wait(lock, [this] {
    return stop_flag || !task_queue.empty();
});
// Thread sleeps until condition is met
```

#### Atomic Variables (std::atomic)
Used for thread-safe counters without mutex overhead.

```cpp
std::atomic<int> active_threads;  // Thread-safe increment/decrement
```

### Real-world example

Like a traffic light system - cars (threads) must synchronize to avoid crashes (race conditions).

### Code Location
- `src/ThreadPool.cpp` - Condition variables
- `src/Account.cpp` - Mutex synchronization

---

## 4. DEADLOCK PREVENTION

### What is it?

A **deadlock** occurs when threads wait for each other forever, none can proceed.

### Classic Deadlock Scenario

```
Transaction 1: Transfer ACC001 → ACC002
- Locks ACC001
- Waits for ACC002

Transaction 2: Transfer ACC002 → ACC001
- Locks ACC002  
- Waits for ACC001

Both stuck forever! ❌ DEADLOCK
```

### Deadlock Conditions (ALL must be true)

1. **Mutual Exclusion**: Resources can't be shared
2. **Hold and Wait**: Thread holds one resource while waiting for another
3. **No Preemption**: Resources can't be forcibly taken
4. **Circular Wait**: Thread A waits for B, B waits for A

### Our Solution: ORDERED LOCKING

**Break condition #4 (Circular Wait)**

```cpp
// In Transaction.cpp
void Transaction::lockAccountsInOrder(Account* acc1, Account* acc2) {
    // ALWAYS lock lower ID first
    if (acc1->getId() < acc2->getId()) {
        acc1->lock();
        acc2->lock();
    } else {
        acc2->lock();
        acc1->lock();
    }
}
```

**Result:**
```
Transaction 1: Transfer ACC001 (ID=1) → ACC002 (ID=2)
- Locks ID=1 first
- Then locks ID=2

Transaction 2: Transfer ACC002 (ID=2) → ACC001 (ID=1)
- Locks ID=1 first (same as T1!)
- Then locks ID=2

No circular wait! ✅ NO DEADLOCK
```

### Real-world example

Like traffic rules: always merge from right (consistent order prevents circular waiting).

### Code Location
- `src/Transaction.cpp` - `lockAccountsInOrder()` method
- `src/main.cpp` - Demo 1 shows deadlock prevention

---

## 5. PROCESS SCHEDULING

### What is it?

**Process scheduling** is how the OS decides which thread runs on which CPU core and when.

### Why do we need it?

With 4 CPU cores but 20 transactions, we need a scheduler to:
- Distribute work fairly
- Keep all cores busy
- Minimize waiting time

### Our Thread Pool Scheduler

```
20 Transactions submitted
        ↓
    [Task Queue]
        ↓
┌────┬────┬────┬────┐
│ T1 │ T2 │ T3 │ T4 │  ← 4 Worker Threads
└────┴────┴────┴────┘
  ↓    ↓    ↓    ↓
Execute transactions
```

### Scheduling Algorithm

We use **First-Come-First-Served (FCFS)** with a queue:

```cpp
// In ThreadPool.cpp
std::queue<std::function<void()>> task_queue;

// Workers take tasks from front of queue
task = task_queue.front();
task_queue.pop();
```

### Metrics We Track

1. **Queue Size**: How many tasks waiting
2. **Active Threads**: How many threads working
3. **Throughput**: Transactions per second
4. **Waiting Time**: Time in queue before execution

### Real-world example

Like a bank with 4 tellers (threads) and one queue - customers (tasks) are served in order.

### Alternative Scheduling Algorithms

#### Round Robin (RR)
Give each thread a time slice, rotate.

#### Priority Scheduling
VIP accounts get priority over regular accounts.

#### Shortest Job First (SJF)
Small transactions processed before large ones.

### Code Location
- `src/ThreadPool.cpp` - Complete scheduler implementation
- `src/main.cpp` - Demo 3 shows scheduling in action

---

## 6. REAL-WORLD APPLICATIONS

### Banking Industry

**Scenario**: HDFC Bank processes millions of transactions daily

**Our Implementation**:
- Each ATM withdrawal = Transaction in queue
- Each teller = Worker thread
- Account balance = Mutex-protected resource
- Transfers between accounts = Deadlock prevention needed

### E-commerce (Amazon, Flipkart)

**Parallel**:
- Product inventory = Bank account balance
- Multiple customers buying = Multiple threads
- Stock updates = Mutex-protected critical section
- Order processing = Thread pool scheduling

### Database Systems (MySQL, PostgreSQL)

**Direct Mapping**:
- Database connections = Threads
- Table locks = Mutex
- Transaction isolation = Synchronization
- Concurrent queries = Deadlock prevention

### Operating System Itself

**Meta Application**:
- Our thread pool = OS process scheduler
- Our mutex = OS kernel mutex
- Our deadlock prevention = OS resource allocation graph

---

## Concept Comparison Table

| Concept | Without It | With It | Cost |
|---------|-----------|---------|------|
| **Threads** | Sequential (slow) | Parallel (fast) | Context switching overhead |
| **Mutex** | Race conditions | Data safety | Lock contention |
| **Synchronization** | Chaos | Coordination | Complexity |
| **Deadlock Prevention** | Stuck forever | Always progressing | Lock ordering overhead |
| **Scheduling** | Unorganized | Efficient distribution | Queue management |

---

## Learning Path

### Beginner → Intermediate

1. ✅ **Week 1** (Current): Core concepts
   - Threads, Mutex, Synchronization
   - Deadlock prevention, Scheduling

2. **Week 2-3**: Intermediate
   - Producer-Consumer problem
   - Readers-Writers problem
   - Bounded buffers

3. **Week 4-6**: Advanced
   - Inter-Process Communication (IPC)
   - Shared memory
   - Memory-mapped files

4. **Week 7-8**: Production-Ready
   - CPU affinity
   - Lock-free data structures
   - Performance optimization

### Questions to Test Understanding

1. What happens if we remove the mutex from `Account::deposit()`?
   → **Answer**: Race conditions, lost updates

2. Why do we lock accounts in order by ID?
   → **Answer**: Prevents circular wait (deadlock condition #4)

3. What if we used one thread instead of a thread pool?
   → **Answer**: Sequential processing, much slower

4. Can two threads read the same account balance simultaneously?
   → **Answer**: No, even reads need mutex (prevent reading during write)

5. What happens if Transaction 1 locks A then B, and Transaction 2 locks B then A?
   → **Answer**: DEADLOCK (circular wait)

---

## Further Reading

### Books
- "Operating System Concepts" by Silberschatz (Chapters 4, 5, 6, 7)
- "Modern Operating Systems" by Tanenbaum (Chapters 2, 6)

### Online Resources
- [pthread tutorial](https://www.cs.cmu.edu/afs/cs/academic/class/15492-f07/www/pthreads.html)
- [C++ Threading](https://en.cppreference.com/w/cpp/thread)
- [Deadlock Prevention](https://www.geeksforgeeks.org/deadlock-prevention/)

### Next Week Preview

**Week 2** adds:
- PostgreSQL database integration
- Connection pooling (resource management)
- Transaction logging (persistence)
- Performance benchmarking

---

## Glossary

**Thread**: Lightweight process, shares memory with other threads  
**Mutex**: Mutual exclusion lock  
**Critical Section**: Code that must not be executed by multiple threads simultaneously  
**Race Condition**: Bug when outcome depends on thread scheduling  
**Deadlock**: Threads waiting for each other forever  
**Synchronization**: Coordinating threads  
**Scheduling**: Deciding which thread runs when  
**Context Switch**: OS switching between threads  
**Concurrency**: Multiple tasks in progress  
**Parallelism**: Multiple tasks executing simultaneously  

---

*For hands-on practice, run the demos in main.cpp and watch the concepts in action!*
