#ifndef TRANSACTION_H
#define TRANSACTION_H

/*
 * Transaction — Week 1 + Week 2
 *
 * Week 1: execute with deadlock prevention
 * Week 2: log every transaction to MySQL
 *
 * OS Concepts:
 *   DEADLOCK PREVENTION — lock accounts in ascending ID order
 *   MUTEX               — via Account's lock/unlock
 *   SEMAPHORE           — via ConnectionPool for DB logging
 */

#include "Account.h"
#include "ConnectionPool.h"
#include <memory>
#include <chrono>
#include <thread>

enum class TxType   { DEPOSIT, WITHDRAW, TRANSFER };
enum class TxStatus { SUCCESS, FAILED };

class Transaction {
private:
    long   id;
    TxType type;
    std::shared_ptr<Account> from;
    std::shared_ptr<Account> to;
    double   amount;
    TxStatus status;
    ConnectionPool* pool;

    // OS: DEADLOCK PREVENTION — always lock lower ID first
    void lockInOrder(Account* a, Account* b);
    void unlockBoth (Account* a, Account* b);

    static std::string typeStr(TxType t);

public:
    Transaction(long id,
                TxType type,
                std::shared_ptr<Account> from,
                std::shared_ptr<Account> to,
                double amount,
                ConnectionPool* pool);

    bool     execute();
    TxStatus getStatus() const { return status; }
    long     getId()     const { return id; }
};

#endif
