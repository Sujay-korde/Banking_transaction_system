#include "ConnectionPool.h"

ConnectionPool::ConnectionPool(const std::string& h, const std::string& u,
                                const std::string& p, const std::string& db,
                                int size, int prt)
    : host(h), user(u), password(p), database(db), port(prt), pool_size(size) {

    sem_init(&semaphore, 0, pool_size);

    std::cout << "\n[POOL] Initializing connection pool (size=" << size << ")...\n";

    for (int i = 0; i < size; i++) {
        auto db_conn = std::make_shared<DatabaseManager>();
        if (db_conn->connect(host, user, password, database, port)) {
            all_connections.push_back(db_conn);
            available.push(db_conn);
            std::cout << "[POOL] Connection " << (i+1) << " established ✓\n";
        } else {
            std::cerr << "[POOL] Failed to create connection " << (i+1) << "\n";
        }
    }

    std::cout << "[POOL] Ready with " << available.size() << "/" << size
              << " connections\n\n";
}

ConnectionPool::~ConnectionPool() {
    sem_destroy(&semaphore);
}

std::shared_ptr<DatabaseManager> ConnectionPool::acquire() {
    int sem_val;
    sem_getvalue(&semaphore, &sem_val);
    if (sem_val == 0) {
        times_blocked++;
        // Only print blocking message during semaphore demo
        // (visible in Demo 2, silent during transaction demos)
        std::cout << "[POOL] Thread blocked — waiting for free connection...\n";
    }

    sem_wait(&semaphore);

    std::lock_guard<std::mutex> lock(pool_mutex);

    if (available.empty()) {
        sem_post(&semaphore);
        throw std::runtime_error("[POOL] No connections available!");
    }

    auto conn = available.front();
    available.pop();
    total_acquired++;
    // Silent acquire — no print (reduces noise during transaction demos)
    return conn;
}

void ConnectionPool::release(std::shared_ptr<DatabaseManager> conn) {
    {
        std::lock_guard<std::mutex> lock(pool_mutex);
        available.push(conn);
        total_released++;
    }
    sem_post(&semaphore);
    // Silent release — no print
}

int ConnectionPool::getAvailable() {
    std::lock_guard<std::mutex> lock(pool_mutex);
    return available.size();
}

void ConnectionPool::printStats() const {
    std::cout << "\n[POOL STATS]\n";
    std::cout << "  Pool size      : " << pool_size << "\n";
    std::cout << "  Total acquired : " << total_acquired << "\n";
    std::cout << "  Total released : " << total_released << "\n";
    std::cout << "  Times blocked  : " << times_blocked
              << " (threads waited for connection)\n";
}
