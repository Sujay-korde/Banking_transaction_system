#include "Transaction.h"
#include <iostream>
#include <thread>

// Global flag - set to false for fast demos, true for persistence demo
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
    auto start = std::chrono::high_resolution_clock::now();
    int tid = (int)(std::hash<std::thread::id>{}(
                    std::this_thread::get_id()) % 100000);
    bool ok = false;

    // ── Execute transaction (always fast, in-memory) ──
    switch (type) {
        case TxType::DEPOSIT:
            ok = to->deposit(amount);
            break;

        case TxType::WITHDRAW:
            ok = from->withdraw(amount);
            break;

        case TxType::TRANSFER:
            // OS: DEADLOCK PREVENTION
            lockInOrder(from.get(), to.get());
            if (from->getBalance() >= amount) {
                from->withdraw(amount);
                to->deposit(amount);
                ok = true;
            }
            unlockBoth(from.get(), to.get());
            break;
    }

    status = ok ? TxStatus::SUCCESS : TxStatus::FAILED;

    // ── DB logging only when enabled ──
    // Disabled during speed demos, enabled for persistence demo
    if (DB_LOGGING_ENABLED) {
        auto end = std::chrono::high_resolution_clock::now();
        int  ms  = (int)std::chrono::duration_cast<
                        std::chrono::milliseconds>(end - start).count();

        std::string from_acc   = from ? from->getAccountNumber() : "";
        std::string to_acc_str = to   ? to->getAccountNumber()   : "";
        std::string type_str   = typeStr(type);
        std::string status_str = ok ? "SUCCESS" : "FAILED";
        double amt = amount;
        bool ok_copy = ok;

        std::thread([this, from_acc, to_acc_str, type_str, amt,
                     status_str, tid, ms, ok_copy]() {
            try {
                auto conn = pool->acquire();
                long long lid = conn->logTransaction(
                    from_acc, to_acc_str, type_str, amt,
                    status_str, tid, ms
                );
                conn->logEvent(lid, ok_copy ? "Completed" : "Failed");
                pool->release(conn);
            } catch (...) {}
        }).detach();
    }

    return ok;
}
