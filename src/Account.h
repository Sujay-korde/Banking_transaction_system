#ifndef ACCOUNT_H
#define ACCOUNT_H

/*
 * Account — Week 1 + Week 2
 *
 * Week 1: in-memory balance, mutex protection
 * Week 2: every balance change persisted to MySQL via ConnectionPool
 *
 * OS Concepts:
 *   MUTEX     — protects balance from race conditions (Week 1)
 *   SEMAPHORE — ConnectionPool uses sem_wait/sem_post to limit
 *               concurrent DB connections (Week 2)
 */

#include <string>
#include <mutex>
#include <iostream>
#include <iomanip>
#include "ConnectionPool.h"

class Account {
private:
    long        id;
    std::string account_number;
    std::string type;
    double      balance;

    std::mutex  acc_mutex;    // OS: MUTEX — critical section guard
    ConnectionPool* pool;     // OS: SEMAPHORE inside pool

    // Save current balance to MySQL
    void persistBalance();

public:
    Account(long id,
            const std::string& account_number,
            const std::string& type,
            double initial_balance,
            ConnectionPool* pool);

    bool   deposit(double amount);
    bool   withdraw(double amount);
    double getBalance();

    // Manual lock/unlock for deadlock prevention in transfers
    void lock()   { acc_mutex.lock(); }
    void unlock() { acc_mutex.unlock(); }

    long        getId()           const { return id; }
    std::string getAccountNumber()const { return account_number; }
    std::string getType()         const { return type; }

    void displayInfo();
};

#endif
