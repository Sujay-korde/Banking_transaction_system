#include "Account.h"

extern bool DB_LOGGING_ENABLED;

Account::Account(long id,
                 const std::string& acc_num,
                 const std::string& t,
                 double initial_balance,
                 ConnectionPool* p)
    : id(id), account_number(acc_num), type(t),
      balance(initial_balance), pool(p) {

    // Always sync with DB on startup
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
    // Only called when DB_LOGGING_ENABLED — synchronous, no threads
    if (!DB_LOGGING_ENABLED) return;
    try {
        auto conn = pool->acquire();
        conn->updateBalance(account_number, balance);
        pool->release(conn);
    } catch (...) {}
}

bool Account::deposit(double amount) {
    if (amount <= 0) return false;
    std::lock_guard<std::mutex> lock(acc_mutex);
    balance += amount;
    persistBalance();
    return true;
}

bool Account::withdraw(double amount) {
    if (amount <= 0) return false;
    std::lock_guard<std::mutex> lock(acc_mutex);
    if (balance < amount) return false;
    balance -= amount;
    persistBalance();
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
