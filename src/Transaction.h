#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "Account.h"
#include <memory>  // For std::shared_ptr

/*
 * Transaction Types
 */
enum class TransactionType {
    DEPOSIT,   // Add money to account
    WITHDRAW,  // Remove money from account
    TRANSFER   // Move money between accounts
};

/*
 * Transaction Status
 */
enum class TransactionStatus {
    PENDING,      // Not yet executed
    SUCCESS,      // Completed successfully
    FAILED,       // Failed (insufficient balance, etc.)
    ROLLED_BACK   // Reversed due to error
};

/*
 * Transaction Class
 * 
 * Represents a single banking transaction.
 * 
 * OS Concepts Demonstrated:
 * - DEADLOCK PREVENTION: Always locks accounts in order
 * - SYNCHRONIZATION: Coordinates multiple threads accessing same accounts
 * - CRITICAL SECTION: Transfer operation is atomic
 */
class Transaction {
private:
    long transaction_id;
    TransactionType type;
    std::shared_ptr<Account> from_account;  // Source account (for withdraw/transfer)
    std::shared_ptr<Account> to_account;    // Destination account (for deposit/transfer)
    double amount;
    TransactionStatus status;
    
    /*
     * OS Concept: DEADLOCK PREVENTION
     * 
     * Problem: If Thread 1 transfers A→B and Thread 2 transfers B→A:
     * - Thread 1 locks A, waits for B
     * - Thread 2 locks B, waits for A
     * - DEADLOCK! Both stuck forever
     * 
     * Solution: Always lock accounts in same order (by ID)
     */
    void lockAccountsInOrder(Account* acc1, Account* acc2);
    void unlockAccounts(Account* acc1, Account* acc2);

public:
    // Constructor
    Transaction(long id, TransactionType t, 
                std::shared_ptr<Account> from, 
                std::shared_ptr<Account> to, 
                double amt);
    
    // Execute the transaction
    bool execute();
    
    // Getters
    TransactionStatus getStatus() const { return status; }
    long getId() const { return transaction_id; }
    TransactionType getType() const { return type; }
    double getAmount() const { return amount; }
    
    // Helper to convert enum to string for display
    static std::string typeToString(TransactionType type);
    static std::string statusToString(TransactionStatus status);
};

#endif // TRANSACTION_H
