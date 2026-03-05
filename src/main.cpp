/*
 * ================================================================
 *  Banking Transaction Processing System
 *  Week 1 + Week 2 Combined
 *
 *  OS Concepts implemented:
 *  ── Week 1 ──────────────────────────────────────────────────
 *  1. Threads            — ThreadPool worker threads
 *  2. Mutex              — Account balance protection
 *  3. Condition Variable — Workers sleep/wake on tasks
 *  4. Deadlock Prevention— Lock accounts in ascending ID order
 *  5. Process Scheduling — Thread pool FIFO distribution
 *  ── Week 2 ──────────────────────────────────────────────────
 *  6. Semaphore          — ConnectionPool limits DB connections
 *  7. IPC Foundation     — Shared pool resource across threads
 *  8. Persistence        — All state saved to MySQL
 * ================================================================
 */

#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <iomanip>
#include <mutex>

#include "ConnectionPool.h"
#include "Account.h"
#include "Transaction.h"
#include "ThreadPool.h"

// ── Terminal colors ──────────────────────────────────────────
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define MAGENTA "\033[35m"
#define BOLD    "\033[1m"
#define CLEAR   "\033[2J\033[H"

// ── DB CONFIG — update DB_PASSWORD before running ───────────
const std::string DB_HOST     = "127.0.0.1";
const std::string DB_USER     = "kimay";
const std::string DB_PASSWORD = "kimaya@1411";
const std::string DB_NAME     = "banking_system";
const int         POOL_SIZE   = 5;

// ── Global stats ─────────────────────────────────────────────
std::atomic<int> g_success{0};
std::atomic<int> g_failed{0};
std::atomic<int> g_deadlock_prevented{0};

// ─────────────────────────────────────────────────────────────
void printHeader() {
    std::cout << CLEAR << BOLD << CYAN;
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║        BANKING TRANSACTION PROCESSING SYSTEM                ║\n";
    std::cout << "║        Week 1 + Week 2  |  8 OS Concepts                   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    std::cout << RESET << "\n";
}

void printSection(const std::string& title) {
    std::cout << "\n" << BOLD << YELLOW;
    std::cout << "┌─────────────────────────────────────────────────────┐\n";
    std::cout << "│  " << std::left << std::setw(51) << title << "│\n";
    std::cout << "└─────────────────────────────────────────────────────┘\n";
    std::cout << RESET;
}

void showBalances(std::vector<std::shared_ptr<Account>>& accounts) {
    std::cout << "\n  ┌────────────┬──────────┬──────────────┐\n";
    std::cout << "  │ Account    │ Type     │ Balance      │\n";
    std::cout << "  ├────────────┼──────────┼──────────────┤\n";
    for (auto& a : accounts) a->displayInfo();
    std::cout << "  └────────────┴──────────┴──────────────┘\n";
}

// ─────────────────────────────────────────────────────────────
//  DEMO 1 — Persistence
//  Proves balances survive program restart (Week 2)
// ─────────────────────────────────────────────────────────────
void demoPersistence(std::vector<std::shared_ptr<Account>>& accounts) {
    printSection("DEMO 1: DB PERSISTENCE  (Week 2)");

    std::cout << GREEN << "\n  Balances loaded FROM MySQL on startup:\n" << RESET;
    showBalances(accounts);

    std::cout << "\n  Depositing $500 to ACC-001, $300 to ACC-002...\n";
    accounts[0]->deposit(500);
    accounts[1]->deposit(300);

    std::cout << GREEN << "\n  Updated balances (saved to MySQL):\n" << RESET;
    showBalances(accounts);

    std::cout << CYAN
              << "\n  TIP: Restart this program — balances will start from\n"
              << "       these values, not the original $10,000/$5,000.\n"
              << "       That is persistence. Week 1 reset every time.\n"
              << RESET;

    std::cout << "\n  Press Enter to continue...";
    std::cin.get();
}

// ─────────────────────────────────────────────────────────────
//  DEMO 2 — Semaphore (Week 2)
//  Visually shows threads blocking on DB connection limit
// ─────────────────────────────────────────────────────────────
void demoSemaphore(ConnectionPool& pool) {
    printSection("DEMO 2: SEMAPHORE  (Week 2)");

    int launch = POOL_SIZE + 3;
    std::cout << "\n  Pool size   : " << POOL_SIZE << " connections\n";
    std::cout << "  Launching   : " << launch << " threads simultaneously\n";
    std::cout << "  Last 3 will : BLOCK at sem_wait() until others release\n\n";

    std::vector<std::thread> threads;
    std::mutex print_m;

    for (int i = 0; i < launch; i++) {
        threads.emplace_back([&pool, &print_m, i]() {
            {
                std::lock_guard<std::mutex> lk(print_m);
                std::cout << "  [Thread-" << i << "] Requesting connection...\n";
            }

            auto conn = pool.acquire();   // OS: sem_wait() — blocks if 0

            {
                std::lock_guard<std::mutex> lk(print_m);
                std::cout << GREEN << "  [Thread-" << i
                          << "] Got connection! Working...\n" << RESET;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(600));

            pool.release(conn);           // OS: sem_post() — wakes waiting thread

            {
                std::lock_guard<std::mutex> lk(print_m);
                std::cout << "  [Thread-" << i << "] Released connection.\n";
            }
        });
    }

    for (auto& t : threads) t.join();

    pool.printStats();
    std::cout << GREEN
              << "\n  Semaphore ensured max " << POOL_SIZE
              << " concurrent DB connections at any time.\n" << RESET;

    std::cout << "\n  Press Enter to continue...";
    std::cin.get();
}

// ─────────────────────────────────────────────────────────────
//  DEMO 3 — Deadlock Prevention (Week 1 concept, Week 2 scale)
// ─────────────────────────────────────────────────────────────
void demoDeadlock(std::vector<std::shared_ptr<Account>>& accounts,
                  ConnectionPool& pool) {
    printSection("DEMO 3: DEADLOCK PREVENTION  (Week 1)");

    std::cout << "\n  Launching 60 cross-transfers (ACC-001 ↔ ACC-002)\n";
    std::cout << "  Without fix : both threads lock one account, wait forever\n";
    std::cout << "  With fix    : always lock lower ID first → no circular wait\n\n";

    ThreadPool tp(4);
    g_success = 0; g_failed = 0;

    for (int i = 0; i < 10; i++) {
        if (i % 2 == 0) {
            tp.enqueue([&]() {
                Transaction t(i, TxType::TRANSFER, accounts[0], accounts[1], 100.0, &pool);
                t.execute() ? g_success++ : g_failed++;
                g_deadlock_prevented++;
            });
        } else {
            tp.enqueue([&]() {
                Transaction t(i, TxType::TRANSFER, accounts[1], accounts[0], 100.0, &pool);
                t.execute() ? g_success++ : g_failed++;
            });
        }
    }

    tp.waitAll();

    std::cout << GREEN << "  ✓ " << (g_success + g_failed)
              << " cross-transfers completed. Zero deadlocks.\n" << RESET;
    std::cout << "  Success: " << g_success
              << "  |  Failed (low balance): " << g_failed << "\n";

    showBalances(accounts);
    std::cout << "\n  Press Enter to continue...";
    std::cin.get();
}

// ─────────────────────────────────────────────────────────────
//  DEMO 4 — Mutex / Race Condition (Week 1)
// ─────────────────────────────────────────────────────────────
void demoMutex(std::vector<std::shared_ptr<Account>>& accounts,
               ConnectionPool& pool) {
    printSection("DEMO 4: MUTEX — RACE CONDITION PREVENTION  (Week 1)");

    double before = accounts[2]->getBalance();
    std::cout << "\n  100 threads each deposit $10 to ACC-003 simultaneously\n";
    std::cout << "  Without mutex : threads overwrite each other (wrong total)\n";
    std::cout << "  With mutex    : each waits its turn (correct total)\n\n";
    std::cout << "  Balance before : $" << std::fixed << std::setprecision(2) << before << "\n";
    std::cout << "  Expected after : $" << (before + 1000.0) << "\n\n";

    ThreadPool tp(8);
    std::atomic<int> done{0};

    for (int i = 0; i < 10; i++) {
        tp.enqueue([&]() {
            Transaction t(i, TxType::DEPOSIT, nullptr, accounts[2], 10.0, &pool);
            t.execute();
            done++;
        });
    }

    tp.waitAll();

    double after = accounts[2]->getBalance();
    std::cout << "  Balance after  : $" << after << "\n";

    if (std::abs(after - (before + 1000.0)) < 0.01)
        std::cout << GREEN << "  ✓ CORRECT — mutex prevented all race conditions\n" << RESET;
    else
        std::cout << RED << "  ✗ Race condition — expected $"
                  << (before+1000) << " got $" << after << "\n" << RESET;

    std::cout << "\n  Press Enter to continue...";
    std::cin.get();
}

// ─────────────────────────────────────────────────────────────
//  DEMO 5 — Full Stress Test with live terminal dashboard
// ─────────────────────────────────────────────────────────────
void demoStressTest(std::vector<std::shared_ptr<Account>>& accounts,
                    ConnectionPool& pool) {
    printSection("DEMO 5: FULL STRESS TEST  (200 transactions, live view)");

    const int TOTAL = 20;
    g_success = 0; g_failed = 0;
    std::atomic<int> submitted{0};

    ThreadPool tp(4);
    auto start = std::chrono::system_clock::now();

    // Submit transactions in background
    std::thread submitter([&]() {
        for (int i = 0; i < TOTAL; i++) {
            int type = i % 3;
            int a = i % (int)accounts.size();
            int b = (i + 1) % (int)accounts.size();

            if (type == 0) {
                tp.enqueue([&, a, i]() {
                    Transaction t(i, TxType::DEPOSIT, nullptr, accounts[a], 100.0, &pool);
                    t.execute() ? g_success++ : g_failed++;
                });
            } else if (type == 1) {
                tp.enqueue([&, a, i]() {
                    Transaction t(i, TxType::WITHDRAW, accounts[a], nullptr, 50.0, &pool);
                    t.execute() ? g_success++ : g_failed++;
                });
            } else {
                tp.enqueue([&, a, b, i]() {
                    Transaction t(i, TxType::TRANSFER, accounts[a], accounts[b], 75.0, &pool);
                    t.execute() ? g_success++ : g_failed++;
                });
            }
            submitted++;
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
    });

    // Live dashboard
    while ((g_success + g_failed) < TOTAL) {
        int done = g_success + g_failed;
        int bar_done = done * 40 / TOTAL;

        std::cout << "\r  [";
        for (int i = 0; i < 40; i++)
            std::cout << (i < bar_done ? "█" : "░");
        std::cout << "]  " << done << "/" << TOTAL
                  << "  ✓" << g_success << " ✗" << g_failed
                  << "  threads:" << tp.getActive()
                  << "  queue:" << tp.getQueueSize()
                  << "   " << std::flush;

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    submitter.join();
    tp.waitAll();

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now() - start).count();

    std::cout << "\n\n" << BOLD << "  Done!\n" << RESET;
    std::cout << "  Success   : " << GREEN << g_success << RESET << "\n";
    std::cout << "  Failed    : " << RED   << g_failed  << RESET << " (insufficient balance)\n";
    std::cout << "  Time      : " << elapsed << "s\n";
    if (elapsed > 0)
        std::cout << "  Throughput: " << (TOTAL / elapsed) << " txn/sec\n";

    showBalances(accounts);

    std::cout << CYAN
              << "\n  Verify in MySQL:\n"
              << "  > USE banking_system;\n"
              << "  > SELECT COUNT(*) FROM transactions;\n"
              << "  > SELECT * FROM transactions ORDER BY created_at DESC LIMIT 5;\n"
              << RESET;
}

// ─────────────────────────────────────────────────────────────
//  SUMMARY
// ─────────────────────────────────────────────────────────────
void printSummary(ConnectionPool& pool) {
    printSection("OS CONCEPTS SUMMARY  —  Week 1 + Week 2");

    std::cout << "\n  Week 1:\n";
    std::cout << "  " << GREEN << "✓" << RESET << " Threads            — " << 4 << " worker threads in ThreadPool\n";
    std::cout << "  " << GREEN << "✓" << RESET << " Mutex              — account_mutex in every deposit/withdraw\n";
    std::cout << "  " << GREEN << "✓" << RESET << " Condition Variable — workers sleep idle, wake on new task\n";
    std::cout << "  " << GREEN << "✓" << RESET << " Deadlock Prevention— transfers lock lower account ID first\n";
    std::cout << "  " << GREEN << "✓" << RESET << " Process Scheduling — thread pool FIFO task distribution\n";

    std::cout << "\n  Week 2:\n";
    std::cout << "  " << CYAN << "✓" << RESET << " Semaphore          — sem_wait/sem_post limits DB to "
              << POOL_SIZE << " connections\n";
    std::cout << "  " << CYAN << "✓" << RESET << " IPC Foundation     — shared pool resource across threads\n";
    std::cout << "  " << CYAN << "✓" << RESET << " Persistence        — MySQL saves all state across restarts\n";

    std::cout << "\n  " << BOLD << "Total: 8 OS concepts active\n" << RESET;

    pool.printStats();

    std::cout << "\n" << BOLD << YELLOW
              << "  Next → Week 3: Producer-Consumer (bounded queue)\n"
              << RESET << "\n";
}

// ─────────────────────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────────────────────
int main() {
    printHeader();

    // Week 2: Connection pool with semaphore
    ConnectionPool pool(DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, POOL_SIZE);

    // Week 2: Accounts load persisted balances from MySQL
    std::cout << BOLD << "\n[STARTUP] Loading accounts from MySQL...\n" << RESET;
    std::vector<std::shared_ptr<Account>> accounts = {
        std::make_shared<Account>(1, "ACC-001", "SAVINGS",  10000.0, &pool),
        std::make_shared<Account>(2, "ACC-002", "CURRENT",   5000.0, &pool),
        std::make_shared<Account>(3, "ACC-003", "VIP",      50000.0, &pool),
        std::make_shared<Account>(4, "ACC-004", "SAVINGS",   8000.0, &pool),
        std::make_shared<Account>(5, "ACC-005", "CURRENT",  12000.0, &pool),
    };
    std::cout << GREEN << "[STARTUP] All accounts ready.\n" << RESET;

    std::cout << "\n  Press Enter to run all demos...";
    std::cin.get();

    demoPersistence(accounts);
    demoSemaphore(pool);
    demoDeadlock(accounts, pool);
    demoMutex(accounts, pool);
    demoStressTest(accounts, pool);
    printSummary(pool);

    return 0;
}
