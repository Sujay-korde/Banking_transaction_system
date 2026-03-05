#ifndef ACCOUNT_H
#define ACCOUNT_H

/*
 * Account — Week 1 + Week 2 + Week 3
 *
 * OS Concepts:
 *   MUTEX     — protects balance from race conditions
 *   SEMAPHORE — ConnectionPool uses sem_wait/sem_post
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

    std::mutex  acc_mutex;
    ConnectionPool* pool;

    void persistBalance();

public:
    Account(long id,
            const std::string& account_number,
            const std::string& type,
            double initial_balance,
            ConnectionPool* pool);

    // Thread-safe versions (acquire mutex internally)
    bool   deposit(double amount);
    bool   withdraw(double amount);

    // Raw versions — NO mutex (caller must hold lock already)
    // Used by Transaction::TRANSFER which locks via lockInOrder
    void   depositRaw(double amount)  { balance += amount; }
    bool   withdrawRaw(double amount) {
        if (balance < amount) return false;
        balance -= amount;
        return true;
    }

    double getBalance();

    void lock()   { acc_mutex.lock(); }
    void unlock() { acc_mutex.unlock(); }

    long        getId()            const { return id; }
    std::string getAccountNumber() const { return account_number; }
    std::string getType()          const { return type; }

    void displayInfo();
};

#endif
