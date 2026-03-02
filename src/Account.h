#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <string>
#include <mutex>      // For std::mutex (Critical Section Protection)
#include <iostream>

/*
 * Account Class
 * 
 * Represents a bank account with thread-safe operations.
 * 
 * OS Concepts Demonstrated:
 * - MUTEX: Protects balance from race conditions
 * - CRITICAL SECTION: deposit() and withdraw() are critical sections
 * - SYNCHRONIZATION: Multiple threads can safely access same account
 */
class Account {
private:
    // Account information
    long account_id;           // Unique ID for account
    std::string account_number; // Human-readable account number (e.g., "ACC001")
    std::string account_type;   // "SAVINGS", "CURRENT", or "VIP"
    double balance;             // Current balance
    
    // OS Concept: MUTEX for thread safety
    // This ensures only ONE thread can modify balance at a time
    std::mutex account_mutex;   

public:
    // Constructor: Create a new account
    Account(long id, std::string acc_num, std::string type, double initial_balance);
    
    // Thread-safe operations
    bool deposit(double amount);      // Add money to account
    bool withdraw(double amount);     // Remove money from account
    double getBalance();               // Check current balance
    
    // Locking functions for advanced deadlock prevention
    void lock();                       // Manually lock this account
    void unlock();                     // Manually unlock this account
    bool tryLock();                    // Try to lock (returns false if already locked)
    
    // Getters
    long getId() const { return account_id; }
    std::string getAccountNumber() const { return account_number; }
    std::string getType() const { return account_type; }
    
    // Display account info
    void displayInfo() const;
};

#endif // ACCOUNT_H
