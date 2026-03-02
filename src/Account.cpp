#include "Account.h"
#include <iomanip>  // For formatting output

// Constructor: Initialize account with starting values
Account::Account(long id, std::string acc_num, std::string type, double initial_balance)
    : account_id(id), 
      account_number(acc_num), 
      account_type(type), 
      balance(initial_balance) {
    
    std::cout << "[ACCOUNT CREATED] " << account_number 
              << " | Type: " << account_type 
              << " | Initial Balance: $" << std::fixed << std::setprecision(2) 
              << balance << std::endl;
}

/*
 * deposit() - Add money to account
 * 
 * OS Concept: CRITICAL SECTION
 * std::lock_guard automatically locks the mutex when created
 * and unlocks when it goes out of scope (at function end)
 * 
 * This prevents race conditions like:
 * Thread 1 reads balance = 100
 * Thread 2 reads balance = 100
 * Thread 1 adds 50, writes 150
 * Thread 2 adds 30, writes 130  <-- Lost Thread 1's deposit!
 */
bool Account::deposit(double amount) {
    // Validate input
    if (amount <= 0) {
        std::cout << "[DEPOSIT FAILED] " << account_number 
                  << " | Invalid amount: $" << amount << std::endl;
        return false;
    }
    
    // OS Concept: MUTEX LOCK (Critical Section Begins)
    // No other thread can execute this code block until lock is released
    std::lock_guard<std::mutex> lock(account_mutex);
    
    // Safe to modify balance now
    double old_balance = balance;
    balance += amount;
    
    std::cout << "[DEPOSIT SUCCESS] " << account_number 
              << " | Amount: $" << std::fixed << std::setprecision(2) << amount
              << " | Old Balance: $" << old_balance 
              << " | New Balance: $" << balance << std::endl;
    
    return true;
    // Mutex automatically unlocked here when 'lock' goes out of scope
}

/*
 * withdraw() - Remove money from account
 * 
 * OS Concept: CRITICAL SECTION with validation
 * Must check if sufficient balance exists
 */
bool Account::withdraw(double amount) {
    // Validate input
    if (amount <= 0) {
        std::cout << "[WITHDRAW FAILED] " << account_number 
                  << " | Invalid amount: $" << amount << std::endl;
        return false;
    }
    
    // OS Concept: MUTEX LOCK (Critical Section Begins)
    std::lock_guard<std::mutex> lock(account_mutex);
    
    // Check sufficient balance
    if (balance < amount) {
        std::cout << "[WITHDRAW FAILED] " << account_number 
                  << " | Insufficient balance. Requested: $" << std::fixed 
                  << std::setprecision(2) << amount 
                  << " | Available: $" << balance << std::endl;
        return false;
    }
    
    // Safe to modify balance
    double old_balance = balance;
    balance -= amount;
    
    std::cout << "[WITHDRAW SUCCESS] " << account_number 
              << " | Amount: $" << amount 
              << " | Old Balance: $" << old_balance 
              << " | New Balance: $" << balance << std::endl;
    
    return true;
    // Mutex automatically unlocked here
}

/*
 * getBalance() - Thread-safe balance check
 * 
 * Even reading requires a lock to prevent reading
 * while another thread is writing (could read half-updated value)
 */
double Account::getBalance() {
    std::lock_guard<std::mutex> lock(account_mutex);
    return balance;
}

/*
 * Manual locking functions for DEADLOCK PREVENTION
 * Used when we need to lock multiple accounts simultaneously
 */
void Account::lock() {
    account_mutex.lock();
}

void Account::unlock() {
    account_mutex.unlock();
}

bool Account::tryLock() {
    return account_mutex.try_lock();
}

// Display account information
void Account::displayInfo() const {
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "Account Number: " << account_number << std::endl;
    std::cout << "Account Type:   " << account_type << std::endl;
    std::cout << "Balance:        $" << std::fixed << std::setprecision(2) 
              << balance << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
}
