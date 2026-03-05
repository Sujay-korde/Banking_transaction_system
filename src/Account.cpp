#include "Account.h"
#include <thread>

Account::Account(long id,
                 const std::string& acc_num,
                 const std::string& t,
                 double initial_balance,
                 ConnectionPool* p)
    : id(id), account_number(acc_num), type(t),
      balance(initial_balance), pool(p) {

    // Startup: this is synchronous (needed before program runs)
    auto conn = pool->acquire();
    if (!conn->accountExists(account_number)) {
        conn->createAccount(account_number, type, balance);
        std::cout << "[ACCOUNT] Created in DB : " << account_number
                  << " | Type: " << type
                  << " | Balance: $" << std::fixed << std::setprecision(2)
                  << balance << "\n";
    } else {
        balance = conn->getBalance(account_number);
        std::cout << "[ACCOUNT] Loaded from DB: " << account_number
                  << " | Type: " << type
                  << " | Balance: $" << std::fixed << std::setprecision(2)
                  << balance << "\n";
    }
    pool->release(conn);
}

void Account::persistBalance() {
    // OS CONCEPT: Async background thread for DB write
    // Balance is already updated in memory (mutex protected).
    // DB sync happens in background — doesn't slow down transaction.
    double current_balance = balance;
    std::string acc = account_number;

    std::thread([this, acc, current_balance]() {
        try {
            auto conn = pool->acquire();
            conn->updateBalance(acc, current_balance);
            pool->release(conn);
        } catch (...) {}
    }).detach();
}

bool Account::deposit(double amount) {
    if (amount <= 0) return false;

    std::lock_guard<std::mutex> lock(acc_mutex);  // OS: MUTEX
    balance += amount;
    persistBalance();                              // OS: async DB sync

    std::cout << "[DEPOSIT]  " << account_number
              << "  +" << std::fixed << std::setprecision(2) << amount
              << "  =>  $" << balance << "  [DB SAVED]\n";
    return true;
}

bool Account::withdraw(double amount) {
    if (amount <= 0) return false;

    std::lock_guard<std::mutex> lock(acc_mutex);  // OS: MUTEX

    if (balance < amount) {
        std::cout << "[WITHDRAW FAIL] " << account_number
                  << " needs $" << amount
                  << " but has $" << balance << "\n";
        return false;
    }

    balance -= amount;
    persistBalance();                              // OS: async DB sync

    std::cout << "[WITHDRAW] " << account_number
              << "  -" << std::fixed << std::setprecision(2) << amount
              << "  =>  $" << balance << "  [DB SAVED]\n";
    return true;
}

double Account::getBalance() {
    std::lock_guard<std::mutex> lock(acc_mutex);
    return balance;
}

void Account::displayInfo() {
    std::cout << "  | " << std::left  << std::setw(10) << account_number
              << " | " << std::setw(8) << type
              << " | $" << std::right << std::setw(10)
              << std::fixed << std::setprecision(2) << getBalance() << " |\n";
}
