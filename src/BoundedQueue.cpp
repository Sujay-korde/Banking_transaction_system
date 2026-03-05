#include "BoundedQueue.h"

BoundedQueue::BoundedQueue(int size) : max_size(size) {
    // OS: Initialize semaphores
    // empty_slots starts at max_size (all slots empty)
    // full_slots  starts at 0       (nothing to consume)
    sem_init(&empty_slots, 0, max_size);
    sem_init(&full_slots,  0, 0);

    std::cout << "[BOUNDED QUEUE] Initialized with capacity " << max_size << "\n";
    std::cout << "[BOUNDED QUEUE] empty_slots=" << max_size
              << " full_slots=0\n";
}

BoundedQueue::~BoundedQueue() {
    sem_destroy(&empty_slots);
    sem_destroy(&full_slots);
}

void BoundedQueue::produce(const TransactionRequest& req) {
    // OS: SEMAPHORE WAIT on empty slots
    // If queue is full (empty_slots=0), producer BLOCKS here
    int empty_val;
    sem_getvalue(&empty_slots, &empty_val);
    if (empty_val == 0) {
        producer_blocked++;
        std::cout << "[" << req.channel << "] Queue FULL — producer blocking...\n";
    }

    sem_wait(&empty_slots);   // Decrement empty slots (block if 0)

    // OS: MUTEX — safely add to buffer
    {
        std::lock_guard<std::mutex> lock(mutex);
        buffer.push(req);
        total_produced++;
    }

    sem_post(&full_slots);    // Increment full slots (wake consumer)
}

TransactionRequest BoundedQueue::consume() {
    // OS: SEMAPHORE WAIT on full slots
    // If queue is empty (full_slots=0), consumer BLOCKS here
    int full_val;
    sem_getvalue(&full_slots, &full_val);
    if (full_val == 0) {
        consumer_blocked++;
    }

    sem_wait(&full_slots);    // Decrement full slots (block if 0)

    // OS: MUTEX — safely remove from buffer
    TransactionRequest req;
    {
        std::lock_guard<std::mutex> lock(mutex);
        req = buffer.front();
        buffer.pop();
        total_consumed++;
    }

    sem_post(&empty_slots);   // Increment empty slots (wake producer)
    return req;
}

bool BoundedQueue::tryConsume(TransactionRequest& req) {
    // Non-blocking version — returns false if queue empty
    if (sem_trywait(&full_slots) != 0) return false;

    std::lock_guard<std::mutex> lock(mutex);
    if (buffer.empty()) {
        sem_post(&full_slots);
        return false;
    }
    req = buffer.front();
    buffer.pop();
    total_consumed++;
    sem_post(&empty_slots);
    return true;
}

int BoundedQueue::getCurrentSize() {
    std::lock_guard<std::mutex> lock(mutex);
    return buffer.size();
}

void BoundedQueue::printStats() const {
    std::cout << "\n[BOUNDED QUEUE STATS]\n";
    std::cout << "  Max capacity     : " << max_size << "\n";
    std::cout << "  Total produced   : " << total_produced << "\n";
    std::cout << "  Total consumed   : " << total_consumed << "\n";
    std::cout << "  Producer blocked : " << producer_blocked
              << " times (queue was full)\n";
    std::cout << "  Consumer blocked : " << consumer_blocked
              << " times (queue was empty)\n";
}
