#include "Transaction.h"
#include <iostream>

bool DB_LOGGING_ENABLED = false;

Transaction::Transaction(long id, TxType t,
                         std::shared_ptr<Account> f,
                         std::shared_ptr<Account> to_acc,
                         double amt,
                         ConnectionPool* p)
    : id(id), type(t), from(f), to(to_acc),
      amount(amt), status(TxStatus::FAILED), pool(p) {}

void Transaction::lockInOrder(Account* a, Account* b) {
    // OS: DEADLOCK PREVENTION — always lock lower ID first
    if (a->getId() < b->getId()) { a->lock(); b->lock(); }
    else                          { b->lock(); a->lock(); }
}

void Transaction::unlockBoth(Account* a, Account* b) {
    a->unlock();
    b->unlock();
}

std::string Transaction::typeStr(TxType t) {
    if (t == TxType::DEPOSIT)  return "DEPOSIT";
    if (t == TxType::WITHDRAW) return "WITHDRAW";
    return "TRANSFER";
}

bool Transaction::execute() {
    bool ok = false;

    switch (type) {
        case TxType::DEPOSIT:
            // deposit() acquires mutex internally — safe
            ok = to->deposit(amount);
            break;

        case TxType::WITHDRAW:
            // withdraw() acquires mutex internally — safe
            ok = from->withdraw(amount);
            break;

        case TxType::TRANSFER:
            // OS: DEADLOCK PREVENTION
            // lockInOrder acquires BOTH mutexes before we touch balances
            // Then use Raw versions — NO mutex inside (we already hold it)
            // This prevents double-lock deadlock
            lockInOrder(from.get(), to.get());
            ok = from->withdrawRaw(amount);  // No mutex — we hold it
            if (ok) to->depositRaw(amount);  // No mutex — we hold it
            unlockBoth(from.get(), to.get());
            break;
    }

    status = ok ? TxStatus::SUCCESS : TxStatus::FAILED;

    if (DB_LOGGING_ENABLED && pool != nullptr) {
        try {
            auto conn = pool->acquire();
            conn->logTransaction(
                from ? from->getAccountNumber() : "",
                to   ? to->getAccountNumber()   : "",
                typeStr(type), amount,
                ok ? "SUCCESS" : "FAILED", 0, 0
            );
            pool->release(conn);
        } catch (...) {}
    }

    return ok;
}
