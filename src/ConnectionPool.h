#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include "DatabaseManager.h"
#include <vector>
#include <queue>
#include <mutex>
#include <semaphore.h>      // OS Concept: POSIX Semaphores
#include <memory>
#include <string>
#include <iostream>
#include <atomic>

/*
 * ConnectionPool
 *
 * OS Concepts demonstrated:
 * ─────────────────────────────────────────────────────────────
 * SEMAPHORE   : sem_wait() blocks threads when all connections
 *               are in use. sem_post() signals when one is free.
 *               This is counting semaphore — counts available slots.
 *
 * MUTEX       : Protects the queue of available connections from
 *               simultaneous access by multiple threads.
 *
 * WHY needed  : Creating a MySQL connection takes ~50-100ms.
 *               Under 1000 concurrent requests, creating a new
 *               connection per request would collapse the system.
 *               Pool reuses a fixed set of connections safely.
 * ─────────────────────────────────────────────────────────────
 */
class ConnectionPool {
private:
    // All connections (owned by pool)
    std::vector<std::shared_ptr<DatabaseManager>> all_connections;

    // Available connections queue
    std::queue<std::shared_ptr<DatabaseManager>> available;

    // OS Concept: MUTEX — protects the queue
    std::mutex pool_mutex;

    // OS Concept: SEMAPHORE — counts available connections
    // sem_wait() = P() = decrement (blocks at 0)
    // sem_post() = V() = increment (wakes waiting thread)
    sem_t semaphore;

    // DB config
    std::string host, user, password, database;
    int port;
    int pool_size;

    // Stats
    std::atomic<int> total_acquired{0};
    std::atomic<int> total_released{0};
    std::atomic<int> times_blocked{0};  // How many times sem_wait actually blocked

public:
    ConnectionPool(const std::string& host, const std::string& user,
                   const std::string& password, const std::string& database,
                   int pool_size = 5, int port = 3306);

    ~ConnectionPool();

    // Acquire a connection (blocks if none available — OS semaphore)
    std::shared_ptr<DatabaseManager> acquire();

    // Release connection back to pool
    void release(std::shared_ptr<DatabaseManager> conn);

    // Stats
    int getAvailable();
    int getPoolSize() const { return pool_size; }
    int getTimesBlocked() const { return times_blocked.load(); }
    int getTotalAcquired() const { return total_acquired.load(); }

    void printStats() const;
};

#endif
