#include "Account.h"
#include "Transaction.h"
#include "ThreadPool.h"
#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>

/*
 * Banking Transaction Processing System
 * Week 1: Basic Implementation
 * 
 * OS Concepts Demonstrated:
 * 1. THREADS - Multiple transactions processed concurrently
 * 2. MUTEX - Protecting account balance (Critical Section)
 * 3. SYNCHRONIZATION - Coordinating access to shared resources
 * 4. DEADLOCK PREVENTION - Locking accounts in order
 * 5. PROCESS SCHEDULING - Thread pool distributes work
 */

void printBanner() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                        ║\n";
    std::cout << "║     Banking Transaction Processing System             ║\n";
    std::cout << "║     Week 1: Foundation Demo                           ║\n";
    std::cout << "║                                                        ║\n";
    std::cout << "║     OS Concepts: Threading, Mutex, Synchronization    ║\n";
    std::cout << "║                  Deadlock Prevention                  ║\n";
    std::cout << "║                                                        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

void demonstrateDeadlockPrevention(ThreadPool& pool,
                                    std::shared_ptr<Account> acc1,
                                    std::shared_ptr<Account> acc2) {
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "  DEMO 1: DEADLOCK PREVENTION\n";
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "\nScenario: Two simultaneous transfers between same accounts\n";
    std::cout << "Without prevention: DEADLOCK (both threads stuck)\n";
    std::cout << "With prevention: Both succeed (locks acquired in order)\n\n";
    
    std::cout << "Initial Balances:\n";
    std::cout << "  " << acc1->getAccountNumber() << ": $" << acc1->getBalance() << "\n";
    std::cout << "  " << acc2->getAccountNumber() << ": $" << acc2->getBalance() << "\n\n";
    
    std::cout << "Launching transactions...\n\n";
    
    // Transaction 1: acc1 → acc2 ($1000)
    pool.enqueue([&]() {
        auto t1 = std::make_shared<Transaction>(1, TransactionType::TRANSFER, 
                                                 acc1, acc2, 1000.0);
        t1->execute();
    });
    
    // Transaction 2: acc2 → acc1 ($500)
    // Without deadlock prevention, this could deadlock with Transaction 1!
    pool.enqueue([&]() {
        auto t2 = std::make_shared<Transaction>(2, TransactionType::TRANSFER, 
                                                 acc2, acc1, 500.0);
        t2->execute();
    });
    
    // Wait for transactions to complete
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    std::cout << "\n✅ Both transactions completed successfully!\n";
    std::cout << "Final Balances:\n";
    std::cout << "  " << acc1->getAccountNumber() << ": $" << acc1->getBalance() << "\n";
    std::cout << "  " << acc2->getAccountNumber() << ": $" << acc2->getBalance() << "\n";
}

void demonstrateCriticalSection(ThreadPool& pool,
                                 std::shared_ptr<Account> acc) {
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "  DEMO 2: CRITICAL SECTION (Mutex Protection)\n";
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "\nScenario: 10 concurrent deposits to same account\n";
    std::cout << "Without mutex: RACE CONDITION (lost updates)\n";
    std::cout << "With mutex: All deposits recorded correctly\n\n";
    
    double initial_balance = acc->getBalance();
    std::cout << "Initial Balance: $" << initial_balance << "\n";
    std::cout << "Each deposit: $100\n";
    std::cout << "Expected final balance: $" << (initial_balance + 1000) << "\n\n";
    
    std::cout << "Launching 10 concurrent deposits...\n\n";
    
    // Create 10 concurrent deposit transactions
    for (int i = 0; i < 10; i++) {
        pool.enqueue([&, i]() {
            auto t = std::make_shared<Transaction>(100 + i, TransactionType::DEPOSIT,
                                                   nullptr, acc, 100.0);
            t->execute();
        });
    }
    
    // Wait for all deposits to complete
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    std::cout << "\n✅ All deposits completed!\n";
    std::cout << "Final Balance: $" << acc->getBalance() << "\n";
    std::cout << "Expected: $" << (initial_balance + 1000) << "\n";
    
    if (acc->getBalance() == initial_balance + 1000) {
        std::cout << "✅ CORRECT! Mutex prevented race conditions.\n";
    } else {
        std::cout << "❌ ERROR! Race condition occurred.\n";
    }
}

void demonstrateThreadPoolScheduling(ThreadPool& pool,
                                     std::vector<std::shared_ptr<Account>>& accounts) {
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "  DEMO 3: THREAD POOL (Process Scheduling)\n";
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "\nScenario: 20 transactions processed by thread pool\n";
    std::cout << "Shows how OS schedules work across multiple threads\n\n";
    
    std::cout << "Thread Pool Configuration:\n";
    std::cout << "  Total threads: " << pool.getTotalThreads() << "\n";
    std::cout << "  Active threads: " << pool.getActiveThreadCount() << "\n\n";
    
    std::cout << "Submitting 20 transactions...\n\n";
    
    // Create mix of transaction types
    for (int i = 0; i < 20; i++) {
        if (i % 3 == 0) {
            // Deposit
            pool.enqueue([&, i]() {
                auto t = std::make_shared<Transaction>(
                    200 + i, TransactionType::DEPOSIT,
                    nullptr, accounts[i % accounts.size()], 50.0
                );
                t->execute();
            });
        } else if (i % 3 == 1) {
            // Withdraw
            pool.enqueue([&, i]() {
                auto t = std::make_shared<Transaction>(
                    200 + i, TransactionType::WITHDRAW,
                    accounts[i % accounts.size()], nullptr, 30.0
                );
                t->execute();
            });
        } else {
            // Transfer
            int from_idx = i % accounts.size();
            int to_idx = (i + 1) % accounts.size();
            pool.enqueue([&, i, from_idx, to_idx]() {
                auto t = std::make_shared<Transaction>(
                    200 + i, TransactionType::TRANSFER,
                    accounts[from_idx], accounts[to_idx], 25.0
                );
                t->execute();
            });
        }
        
        // Small delay to show queue building up
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        std::cout << "Queue size: " << pool.getQueueSize() 
                  << " | Active: " << pool.getActiveThreadCount() << "\n";
    }
    
    std::cout << "\nWaiting for all transactions to complete...\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    std::cout << "\n✅ All transactions processed!\n";
}

int main() {
    printBanner();
    
    // Create accounts
    std::cout << "Creating accounts...\n\n";
    auto acc1 = std::make_shared<Account>(1, "ACC001", "SAVINGS", 10000.0);
    auto acc2 = std::make_shared<Account>(2, "ACC002", "CURRENT", 5000.0);
    auto acc3 = std::make_shared<Account>(3, "ACC003", "VIP", 50000.0);
    auto acc4 = std::make_shared<Account>(4, "ACC004", "SAVINGS", 8000.0);
    auto acc5 = std::make_shared<Account>(5, "ACC005", "CURRENT", 12000.0);
    
    std::vector<std::shared_ptr<Account>> accounts = {acc1, acc2, acc3, acc4, acc5};
    
    // Create thread pool
    // Using 4 threads for clear demonstration (can see multiple threads working)
    ThreadPool pool(4);
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Demo 1: Deadlock Prevention
    demonstrateDeadlockPrevention(pool, acc1, acc2);
    
    std::cout << "\nPress Enter to continue to next demo...";
    std::cin.get();
    
    // Demo 2: Critical Section (Mutex)
    demonstrateCriticalSection(pool, acc3);
    
    std::cout << "\nPress Enter to continue to next demo...";
    std::cin.get();
    
    // Demo 3: Thread Pool Scheduling
    demonstrateThreadPoolScheduling(pool, accounts);
    
    // Final summary
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "  WEEK 1 COMPLETE - SUMMARY\n";
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "\n✅ OS Concepts Demonstrated:\n";
    std::cout << "   1. THREADS - Concurrent transaction processing\n";
    std::cout << "   2. MUTEX - Critical section protection\n";
    std::cout << "   3. SYNCHRONIZATION - Coordinated account access\n";
    std::cout << "   4. DEADLOCK PREVENTION - Ordered lock acquisition\n";
    std::cout << "   5. PROCESS SCHEDULING - Thread pool work distribution\n";
    std::cout << "\n✅ All demonstrations completed successfully!\n\n";
    
    std::cout << "Final Account Balances:\n";
    for (const auto& acc : accounts) {
        acc->displayInfo();
    }
    
    return 0;
}
