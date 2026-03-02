#include "Transaction.h"
#include <iostream>
#include <thread>    // For std::this_thread
#include <chrono>    // For sleep
#include <iomanip>

// Constructor
Transaction::Transaction(long id, TransactionType t, 
                         std::shared_ptr<Account> from,
                         std::shared_ptr<Account> to, 
                         double amt)
    : transaction_id(id), 
      type(t), 
      from_account(from), 
      to_account(to),
      amount(amt), 
      status(TransactionStatus::PENDING) {
    
    std::cout << "[TRANSACTION CREATED] ID: " << id 
              << " | Type: " << typeToString(type) 
              << " | Amount: $" << std::fixed << std::setprecision(2) << amount 
              << std::endl;
}

/*
 * OS Concept: DEADLOCK PREVENTION
 * 
 * CRITICAL: Always lock accounts in ascending ID order
 * This ensures all threads acquire locks in same sequence
 */
void Transaction::lockAccountsInOrder(Account* acc1, Account* acc2) {
    // Get thread ID for logging
    auto thread_id = std::this_thread::get_id();
    
    // Lock in ascending ID order to prevent circular wait
    if (acc1->getId() < acc2->getId()) {
        std::cout << "[Thread " << thread_id << "] Locking account " 
                  << acc1->getId() << "..." << std::endl;
        acc1->lock();
        
        std::cout << "[Thread " << thread_id << "] Locking account " 
                  << acc2->getId() << "..." << std::endl;
        acc2->lock();
    } else {
        std::cout << "[Thread " << thread_id << "] Locking account " 
                  << acc2->getId() << "..." << std::endl;
        acc2->lock();
        
        std::cout << "[Thread " << thread_id << "] Locking account " 
                  << acc1->getId() << "..." << std::endl;
        acc1->lock();
    }
    
    std::cout << "[Thread " << thread_id << "] ✓ Both accounts locked" << std::endl;
}

void Transaction::unlockAccounts(Account* acc1, Account* acc2) {
    acc1->unlock();
    acc2->unlock();
    
    auto thread_id = std::this_thread::get_id();
    std::cout << "[Thread " << thread_id << "] ✓ Both accounts unlocked" << std::endl;
}

/*
 * execute() - Perform the transaction
 * 
 * OS Concepts:
 * - CRITICAL SECTION: Balance updates are atomic
 * - SYNCHRONIZATION: Multiple threads coordinate safely
 * - DEADLOCK PREVENTION: Locks acquired in order
 */
bool Transaction::execute() {
    status = TransactionStatus::PENDING;
    
    try {
        switch (type) {
            case TransactionType::DEPOSIT: {
                // Simple deposit - only one account involved
                if (to_account && to_account->deposit(amount)) {
                    status = TransactionStatus::SUCCESS;
                    std::cout << "[TRANSACTION " << transaction_id 
                              << "] SUCCESS - Deposit" << std::endl;
                    return true;
                }
                break;
            }
            
            case TransactionType::WITHDRAW: {
                // Simple withdrawal - only one account involved
                if (from_account && from_account->withdraw(amount)) {
                    status = TransactionStatus::SUCCESS;
                    std::cout << "[TRANSACTION " << transaction_id 
                              << "] SUCCESS - Withdraw" << std::endl;
                    return true;
                }
                break;
            }
            
            case TransactionType::TRANSFER: {
                /*
                 * OS Concept: CRITICAL SECTION with DEADLOCK PREVENTION
                 * 
                 * This is the most complex case:
                 * 1. Lock both accounts (in order!)
                 * 2. Check sufficient balance
                 * 3. Perform atomic transfer
                 * 4. Unlock both accounts
                 */
                
                if (!from_account || !to_account) {
                    status = TransactionStatus::FAILED;
                    return false;
                }
                
                std::cout << "[TRANSACTION " << transaction_id 
                          << "] Starting transfer: " 
                          << from_account->getAccountNumber() << " → " 
                          << to_account->getAccountNumber() << std::endl;
                
                // Lock both accounts (DEADLOCK PREVENTION)
                lockAccountsInOrder(from_account.get(), to_account.get());
                
                // Simulate processing time (makes threading visible)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                // Check balance (already inside critical section)
                double from_balance = from_account->getBalance();
                
                if (from_balance >= amount) {
                    // Perform atomic transfer
                    from_account->withdraw(amount);
                    to_account->deposit(amount);
                    
                    status = TransactionStatus::SUCCESS;
                    
                    std::cout << "[TRANSACTION " << transaction_id 
                              << "] SUCCESS - Transfer completed" << std::endl;
                    
                    // Unlock accounts
                    unlockAccounts(from_account.get(), to_account.get());
                    return true;
                    
                } else {
                    // Insufficient balance
                    status = TransactionStatus::FAILED;
                    
                    std::cout << "[TRANSACTION " << transaction_id 
                              << "] FAILED - Insufficient balance. "
                              << "Required: $" << amount 
                              << ", Available: $" << from_balance << std::endl;
                    
                    unlockAccounts(from_account.get(), to_account.get());
                    return false;
                }
            }
        }
        
        // If we reach here, transaction failed
        status = TransactionStatus::FAILED;
        std::cout << "[TRANSACTION " << transaction_id << "] FAILED" << std::endl;
        return false;
        
    } catch (const std::exception& e) {
        status = TransactionStatus::FAILED;
        std::cerr << "[TRANSACTION " << transaction_id 
                  << "] ERROR: " << e.what() << std::endl;
        return false;
    }
}

// Helper functions to convert enums to strings
std::string Transaction::typeToString(TransactionType type) {
    switch (type) {
        case TransactionType::DEPOSIT:  return "DEPOSIT";
        case TransactionType::WITHDRAW: return "WITHDRAW";
        case TransactionType::TRANSFER: return "TRANSFER";
        default: return "UNKNOWN";
    }
}

std::string Transaction::statusToString(TransactionStatus status) {
    switch (status) {
        case TransactionStatus::PENDING:     return "PENDING";
        case TransactionStatus::SUCCESS:     return "SUCCESS";
        case TransactionStatus::FAILED:      return "FAILED";
        case TransactionStatus::ROLLED_BACK: return "ROLLED_BACK";
        default: return "UNKNOWN";
    }
}
