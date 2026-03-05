#ifndef BOUNDED_QUEUE_H
#define BOUNDED_QUEUE_H

/*
 * BoundedQueue — Week 3
 *
 * Implements the classic Producer-Consumer problem using
 * POSIX semaphores and mutex.
 *
 * OS Concepts:
 * ─────────────────────────────────────────────────────────
 * BOUNDED BUFFER    — Fixed capacity queue (MAX_SIZE slots)
 *
 * SEMAPHORE (empty) — Counts empty slots available
 *                     Producer does sem_wait(empty) before adding
 *                     If queue full (empty=0), producer BLOCKS
 *
 * SEMAPHORE (full)  — Counts filled slots available
 *                     Consumer does sem_wait(full) before taking
 *                     If queue empty (full=0), consumer BLOCKS
 *
 * MUTEX             — Protects queue during add/remove
 *
 * WHY needed in banking:
 *   Peak hours → ATM/Mobile/Web flood thousands of requests
 *   Without bounded queue → system runs out of memory
 *   With bounded queue → system accepts only what it can handle
 *   Producers slow down automatically when queue fills up
 * ─────────────────────────────────────────────────────────
 */

#include <queue>
#include <mutex>
#include <semaphore.h>
#include <functional>
#include <string>
#include <atomic>
#include <iostream>

struct TransactionRequest {
    int  id;
    std::string type;      // "DEPOSIT", "WITHDRAW", "TRANSFER"
    std::string channel;   // "ATM", "MOBILE", "WEB"
    std::string from_acc;
    std::string to_acc;
    double amount;
};

class BoundedQueue {
private:
    std::queue<TransactionRequest> buffer;
    int max_size;

    std::mutex mutex;          // OS: MUTEX — protects buffer
    sem_t empty_slots;         // OS: SEMAPHORE — counts empty slots
    sem_t full_slots;          // OS: SEMAPHORE — counts filled slots

    // Stats
    std::atomic<int> total_produced{0};
    std::atomic<int> total_consumed{0};
    std::atomic<int> producer_blocked{0};
    std::atomic<int> consumer_blocked{0};

public:
    BoundedQueue(int max_size);
    ~BoundedQueue();

    // Producer: add transaction to queue
    // BLOCKS if queue is full (sem_wait on empty_slots)
    void produce(const TransactionRequest& req);

    // Consumer: take transaction from queue
    // BLOCKS if queue is empty (sem_wait on full_slots)
    TransactionRequest consume();

    // Non-blocking check
    bool tryConsume(TransactionRequest& req);

    int  getCurrentSize();
    int  getMaxSize() const { return max_size; }
    int  getTotalProduced() const { return total_produced.load(); }
    int  getTotalConsumed() const { return total_consumed.load(); }
    int  getProducerBlocked() const { return producer_blocked.load(); }
    int  getConsumerBlocked() const { return consumer_blocked.load(); }

    void printStats() const;
};

#endif
