#ifndef READWRITELOCK_H
#define READWRITELOCK_H

/*
 * ReadWriteLock — Week 4
 *
 * Implements the Readers-Writers Problem using mutex + semaphore.
 *
 * OS Concepts:
 * ─────────────────────────────────────────────────────────────
 * READERS-WRITERS PROBLEM:
 *   Multiple threads may READ simultaneously (no conflict)
 *   Only ONE thread may WRITE at a time (exclusive access)
 *   A WRITE blocks all new READS until write completes
 *
 * WHY needed in banking:
 *   100s of threads check balances simultaneously → allow all
 *   1 thread does a transfer → block all reads/writes until done
 *   Without this → readers see inconsistent mid-transfer balances
 *
 * Implementation:
 *   read_mutex  — protects reader_count variable
 *   write_sem   — binary semaphore, blocks writers + first reader
 *   reader_count— tracks active readers
 *
 * Key insight:
 *   First reader  → acquires write_sem (blocks writers)
 *   Last reader   → releases write_sem (allows writers)
 *   Middle readers→ just increment/decrement counter
 *   Writer        → acquires write_sem directly (exclusive)
 * ─────────────────────────────────────────────────────────────
 */

#include <mutex>
#include <semaphore.h>
#include <atomic>
#include <iostream>

class ReadWriteLock {
private:
    std::mutex  read_mutex;     // OS: MUTEX — protects reader_count
    sem_t       write_sem;      // OS: SEMAPHORE — writer exclusion
    int         reader_count;   // Active readers right now
    
    // Stats
    std::atomic<int> total_reads{0};
    std::atomic<int> total_writes{0};
    std::atomic<int> concurrent_reads{0};
    std::atomic<int> max_concurrent_reads{0};
    std::atomic<int> writers_blocked{0};

public:
    ReadWriteLock() : reader_count(0) {
        // Binary semaphore — starts at 1 (unlocked)
        sem_init(&write_sem, 0, 1);
    }

    ~ReadWriteLock() {
        sem_destroy(&write_sem);
    }

    // ── Reader Lock ──────────────────────────────────────────
    void readLock() {
        std::lock_guard<std::mutex> lock(read_mutex);  // OS: MUTEX
        reader_count++;
        total_reads++;
        concurrent_reads++;

        int cr = concurrent_reads.load();
        if (cr > max_concurrent_reads.load())
            max_concurrent_reads = cr;

        if (reader_count == 1) {
            // First reader — block writers
            sem_wait(&write_sem);  // OS: SEMAPHORE sem_wait()
        }
        // Other readers enter freely (writer already blocked)
    }

    void readUnlock() {
        std::lock_guard<std::mutex> lock(read_mutex);  // OS: MUTEX
        reader_count--;
        concurrent_reads--;

        if (reader_count == 0) {
            // Last reader — allow writers
            sem_post(&write_sem);  // OS: SEMAPHORE sem_post()
        }
    }

    // ── Writer Lock ──────────────────────────────────────────
    void writeLock() {
        int sem_val;
        sem_getvalue(&write_sem, &sem_val);
        if (sem_val == 0) {
            writers_blocked++;
        }
        sem_wait(&write_sem);  // OS: SEMAPHORE — blocks if readers active
        total_writes++;
    }

    void writeUnlock() {
        sem_post(&write_sem);  // OS: SEMAPHORE — release exclusive access
    }

    void printStats() const {
        std::cout << "\n[READ-WRITE LOCK STATS]\n";
        std::cout << "  Total reads          : " << total_reads << "\n";
        std::cout << "  Total writes         : " << total_writes << "\n";
        std::cout << "  Max concurrent reads : " << max_concurrent_reads << "\n";
        std::cout << "  Writers blocked      : " << writers_blocked
                  << " times (waited for readers)\n";
    }
};

// ── RAII wrappers for safe lock/unlock ───────────────────────
class ReadGuard {
    ReadWriteLock& rwl;
public:
    ReadGuard(ReadWriteLock& r) : rwl(r) { rwl.readLock(); }
    ~ReadGuard() { rwl.readUnlock(); }
};

class WriteGuard {
    ReadWriteLock& rwl;
public:
    WriteGuard(ReadWriteLock& r) : rwl(r) { rwl.writeLock(); }
    ~WriteGuard() { rwl.writeUnlock(); }
};

#endif
