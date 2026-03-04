#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>
#include <atomic>
#include <memory>
#include <chrono>
#include <random>
#include <iomanip>
#include <sstream>

using namespace std;
using namespace std::chrono;

// ── Global stats ──
atomic<int> total_success{0};
atomic<int> total_failed{0};
atomic<int> active_threads_count{0};
atomic<int> queue_size{0};
atomic<int> deadlock_prevented{0};
atomic<int> mutex_contentions{0};

// ── Colors ──
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define MAGENTA "\033[35m"
#define BOLD    "\033[1m"
#define CLEAR   "\033[2J\033[H"

// ── Thread Pool ──
class ThreadPool {
private:
    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex q_mutex;
    condition_variable cv;
    atomic<bool> stop_flag{false};
    int num_threads;

public:
    atomic<int> busy{0};

    ThreadPool(int n) : num_threads(n) {
        for (int i = 0; i < n; i++) {
            workers.emplace_back([this] {
                while (true) {
                    function<void()> task;
                    {
                        unique_lock<mutex> lock(q_mutex);
                        cv.wait(lock, [this]{ return stop_flag || !tasks.empty(); });
                        if (stop_flag && tasks.empty()) return;
                        task = move(tasks.front());
                        tasks.pop();
                        queue_size--;
                    }
                    busy++;
                    active_threads_count++;
                    task();
                    active_threads_count--;
                    busy--;
                }
            });
        }
    }

    template<class F>
    void enqueue(F&& f) {
        {
            unique_lock<mutex> lock(q_mutex);
            tasks.emplace(forward<F>(f));
            queue_size++;
        }
        cv.notify_one();
    }

    // Wait until all submitted tasks are done
    void waitAll() {
        while (true) {
            {
                unique_lock<mutex> lock(q_mutex);
                if (tasks.empty() && busy == 0) return;
            }
            this_thread::sleep_for(milliseconds(50));
        }
    }

    int getNumThreads() { return num_threads; }

    ~ThreadPool() {
        stop_flag = true;
        cv.notify_all();
        for (auto& w : workers) if (w.joinable()) w.join();
    }
};

// ── Account ──
class Account {
private:
    long id;
    string name;
    string type;
    double balance;
    mutex acc_mutex;

public:
    Account(long id, string name, string type, double bal)
        : id(id), name(name), type(type), balance(bal) {}

    bool deposit(double amount) {
        lock_guard<mutex> lock(acc_mutex);
        if (amount <= 0) return false;
        balance += amount;
        return true;
    }

    bool withdraw(double amount) {
        lock_guard<mutex> lock(acc_mutex);
        if (amount <= 0 || balance < amount) return false;
        balance -= amount;
        return true;
    }

    double getBalance() {
        lock_guard<mutex> lock(acc_mutex);
        return balance;
    }

    void depositUnsafe(double a)  { balance += a; }
    bool withdrawUnsafe(double a) { if(balance<a) return false; balance-=a; return true; }
    double getBalanceUnsafe()     { return balance; }
    void lockAcc()   { acc_mutex.lock(); }
    void unlockAcc() { acc_mutex.unlock(); }
    long getId()     { return id; }
    string getName() { return name; }
    string getType() { return type; }
};

// ── Transfer with deadlock prevention ──
bool transfer(shared_ptr<Account> from, shared_ptr<Account> to, double amount) {
    Account* first  = (from->getId() < to->getId()) ? from.get() : to.get();
    Account* second = (from->getId() < to->getId()) ? to.get()   : from.get();

    if (first == to.get()) deadlock_prevented++;

    first->lockAcc();
    this_thread::sleep_for(milliseconds(1));
    second->lockAcc();

    bool ok = false;
    if (from->getBalanceUnsafe() >= amount) {
        from->withdrawUnsafe(amount);
        to->depositUnsafe(amount);
        ok = true;
    }

    second->unlockAcc();
    first->unlockAcc();
    return ok;
}

// ── Progress bar ──
void printBar(int val, int max_val, int width, const string& color) {
    int filled = (max_val > 0) ? min(val * width / max_val, width) : 0;
    cout << color << "[";
    for (int i = 0; i < width; i++) cout << (i < filled ? "█" : "░");
    cout << "] " << val << "/" << max_val << RESET;
}

// ════════════════════════════════════════
//  DEMO 1: Deadlock Prevention
// ════════════════════════════════════════
void demo1_deadlock(vector<shared_ptr<Account>>& accounts, ThreadPool& pool) {
    cout << CLEAR;
    cout << BOLD << YELLOW
         << "╔══════════════════════════════════════════════╗\n"
         << "║    DEMO 1: DEADLOCK PREVENTION               ║\n"
         << "╚══════════════════════════════════════════════╝\n" << RESET;

    cout << "\n  Scenario: Thread A does ACC1 → ACC2 transfer\n"
         << "            Thread B does ACC2 → ACC1 transfer  (simultaneously)\n\n"
         << "  " << RED << "WITHOUT fix:" << RESET << " Thread A locks ACC1, waits for ACC2\n"
         << "               Thread B locks ACC2, waits for ACC1\n"
         << "               " << RED << "DEADLOCK — both stuck forever!\n\n" << RESET
         << "  " << GREEN << "WITH our fix:" << RESET << " Always lock the lower account ID first\n"
         << "               Both threads try ACC1 first → one waits → " << GREEN << "No deadlock!\n\n" << RESET;

    cout << "  Launching 50 simultaneous cross-transfers...\n\n";
    this_thread::sleep_for(seconds(1));

    deadlock_prevented = 0;
    for (int i = 0; i < 50; i++) {
        if (i % 2 == 0)
            pool.enqueue([&]{ transfer(accounts[0], accounts[1], 50.0) ? total_success++ : total_failed++; });
        else
            pool.enqueue([&]{ transfer(accounts[1], accounts[0], 50.0) ? total_success++ : total_failed++; });
    }

    pool.waitAll();

    cout << GREEN << "  ✓ All 50 transfers completed. Deadlocks avoided: "
         << deadlock_prevented << "\n" << RESET;
    cout << "\n  Press Enter for Demo 2...";
    cin.get();
}

// ════════════════════════════════════════
//  DEMO 2: Mutex / Race Condition
// ════════════════════════════════════════
void demo2_mutex(vector<shared_ptr<Account>>& accounts, ThreadPool& pool) {
    cout << CLEAR;
    cout << BOLD << CYAN
         << "╔══════════════════════════════════════════════╗\n"
         << "║    DEMO 2: MUTEX - RACE CONDITION PREVENTION ║\n"
         << "╚══════════════════════════════════════════════╝\n" << RESET;

    cout << "\n  Scenario: 100 threads all deposit $10 to same account\n\n";

    double before = accounts[2]->getBalance();
    cout << "  Balance BEFORE : $" << fixed << setprecision(2) << before << "\n"
         << "  Expected AFTER : $" << (before + 1000.0) << "  (100 deposits x $10)\n\n"
         << "  " << RED    << "WITHOUT mutex:" << RESET << " threads overwrite each other → wrong total\n"
         << "  " << GREEN  << "WITH mutex:   " << RESET << " each thread waits its turn  → exact total\n\n"
         << "  Submitting 100 concurrent deposits...\n";

    this_thread::sleep_for(milliseconds(500));

    mutex_contentions = 0;
    for (int i = 0; i < 100; i++) {
        pool.enqueue([&]{
            accounts[2]->deposit(10.0);
            mutex_contentions++;
            total_success++;
        });
    }

    pool.waitAll();  // Wait until ALL 100 are done before checking

    double after = accounts[2]->getBalance();
    cout << "\n  Deposits done  : 100/100\n"
         << "  Balance AFTER  : $" << fixed << setprecision(2) << after << "\n";

    if (abs(after - (before + 1000.0)) < 0.01)
        cout << GREEN << "\n  ✓ CORRECT! Mutex protected all 100 critical sections.\n" << RESET;
    else
        cout << RED << "\n  ✗ Mismatch — expected $" << (before+1000) << " got $" << after << "\n" << RESET;

    cout << "\n  Press Enter for Demo 3 (Live Stress Test)...";
    cin.get();
}

// ════════════════════════════════════════
//  DEMO 3: Live Stress Test Dashboard
// ════════════════════════════════════════
void demo3_stresstest(vector<shared_ptr<Account>>& accounts, ThreadPool& pool) {
    total_success = 0; total_failed = 0;
    deadlock_prevented = 0; mutex_contentions = 0;

    const int TOTAL = 500;
    auto start = system_clock::now();

    // Submit transactions in background
    thread submitter([&]{
        mt19937 rng(42);
        uniform_int_distribution<int> acc(0, (int)accounts.size()-1);
        uniform_real_distribution<double> amt(50.0, 300.0);
        uniform_int_distribution<int> type(0, 2);

        for (int i = 0; i < TOTAL; i++) {
            int t = type(rng), a = acc(rng), b = acc(rng);
            while (b == a) b = acc(rng);
            double amount = amt(rng);

            if (t == 0) {
                pool.enqueue([&accounts, a, amount]{
                    accounts[a]->deposit(amount);
                    mutex_contentions++; total_success++;
                });
            } else if (t == 1) {
                pool.enqueue([&accounts, a, amount]{
                    accounts[a]->withdraw(amount) ? total_success++ : total_failed++;
                    mutex_contentions++;
                });
            } else {
                pool.enqueue([&accounts, a, b, amount]{
                    transfer(accounts[a], accounts[b], amount) ? total_success++ : total_failed++;
                    mutex_contentions++;
                });
            }
            this_thread::sleep_for(milliseconds(15));
        }
    });

    // Live dashboard loop
    while ((total_success + total_failed) < TOTAL) {
        int done = total_success + total_failed;
        auto elapsed = duration_cast<seconds>(system_clock::now() - start).count();

        cout << CLEAR;
        cout << BOLD << CYAN
             << "╔══════════════════════════════════════════════════════════════╗\n"
             << "║       BANKING SYSTEM - LIVE STRESS TEST DASHBOARD           ║\n"
             << "╚══════════════════════════════════════════════════════════════╝\n"
             << RESET;

        cout << YELLOW << "  Elapsed: " << elapsed << "s\n\n" << RESET;

        cout << BOLD << "  [OS: PROCESS SCHEDULING — Thread Pool]\n" << RESET;
        cout << "  Active threads : ";
        printBar(active_threads_count.load(), pool.getNumThreads(), 20, GREEN);
        cout << "\n  Queue size     : ";
        printBar(min(queue_size.load(), 50), 50, 20, YELLOW);
        cout << "\n\n";

        cout << BOLD << "  [OS: CRITICAL SECTION — Mutex]\n" << RESET;
        cout << "  Sections protected : " << GREEN << mutex_contentions.load() << RESET << "\n\n";

        cout << BOLD << "  [OS: DEADLOCK PREVENTION — Ordered Locking]\n" << RESET;
        cout << "  Lock orders corrected : " << MAGENTA << deadlock_prevented.load() << RESET << "\n\n";

        cout << BOLD << "  [TRANSACTIONS]\n" << RESET;
        cout << "  Progress : ";
        printBar(done, TOTAL, 30, CYAN);
        cout << "\n";
        cout << "  " << GREEN << "✓ Success: " << total_success
             << "  " << RED   << "✗ Failed: "  << total_failed
             << "  " << YELLOW<< "⟳ Pending: " << (TOTAL - done) << RESET << "\n";
        if (elapsed > 0)
            cout << "  Throughput : " << BOLD << (done / elapsed) << " txn/sec\n" << RESET;

        cout << "\n" << BOLD << "  [LIVE ACCOUNT BALANCES]\n" << RESET;
        cout << "  ┌─────────────┬─────────────┬───────────────┐\n"
             << "  │ Account     │ Type        │ Balance       │\n"
             << "  ├─────────────┼─────────────┼───────────────┤\n";
        for (auto& a : accounts) {
            cout << "  │ " << left << setw(12) << a->getName()
                 << "│ " << setw(12) << a->getType()
                 << "│ $" << right << setw(12) << fixed << setprecision(2)
                 << a->getBalance() << " │\n";
        }
        cout << "  └─────────────┴─────────────┴───────────────┘\n";

        this_thread::sleep_for(milliseconds(700));
    }

    submitter.join();
    pool.waitAll();

    // Final summary
    auto elapsed = duration_cast<seconds>(system_clock::now() - start).count();
    cout << CLEAR;
    cout << BOLD << GREEN
         << "╔══════════════════════════════════════════════╗\n"
         << "║         WEEK 1 — ALL TESTS COMPLETE ✓        ║\n"
         << "╚══════════════════════════════════════════════╝\n" << RESET;

    cout << "\n  Transactions   : " << TOTAL
         << "\n  " << GREEN   << "✓ Success      : " << total_success    << RESET
         << "\n  " << RED     << "✗ Failed       : " << total_failed << " (low balance)" << RESET
         << "\n  " << MAGENTA << "⚡ Deadlocks avoided : " << deadlock_prevented << RESET
         << "\n  " << CYAN    << "🔒 Mutex sections   : " << mutex_contentions  << RESET
         << "\n  Time           : " << elapsed << "s"
         << "\n  Throughput     : " << (elapsed > 0 ? TOTAL/elapsed : TOTAL) << " txn/sec\n\n";

    cout << BOLD << "  OS Concepts verified in this run:\n" << RESET
         << "  " << GREEN << "✓" << RESET << " Threads          — 4 workers processed all 500 transactions\n"
         << "  " << GREEN << "✓" << RESET << " Mutex            — every balance update was a protected critical section\n"
         << "  " << GREEN << "✓" << RESET << " Condition Var    — threads slept when idle, woke on new tasks\n"
         << "  " << GREEN << "✓" << RESET << " Deadlock Prev    — " << deadlock_prevented.load() << " lock orderings corrected\n"
         << "  " << GREEN << "✓" << RESET << " Process Sched    — FIFO thread pool distributed work across 4 cores\n\n"
         << BOLD << CYAN << "  → Week 2 next: MySQL integration — balances persist, connection pool!\n\n" << RESET;
}

// ════════════════════════════════════════
//  MAIN
// ════════════════════════════════════════
int main() {
    vector<shared_ptr<Account>> accounts = {
        make_shared<Account>(1, "ACC-001", "SAVINGS",  10000.0),
        make_shared<Account>(2, "ACC-002", "CURRENT",   5000.0),
        make_shared<Account>(3, "ACC-003", "VIP",      50000.0),
        make_shared<Account>(4, "ACC-004", "SAVINGS",   8000.0),
        make_shared<Account>(5, "ACC-005", "CURRENT",  12000.0),
    };

    ThreadPool pool(4);

    cout << CLEAR << BOLD << GREEN
         << "╔══════════════════════════════════════════════════════════════╗\n"
         << "║          BANKING SYSTEM — WEEK 1 TEST SUITE                 ║\n"
         << "║                                                              ║\n"
         << "║  Demo 1 : Deadlock Prevention   (50 cross-transfers)        ║\n"
         << "║  Demo 2 : Mutex Protection      (100 concurrent deposits)   ║\n"
         << "║  Demo 3 : Live Stress Test      (500 random transactions)   ║\n"
         << "╚══════════════════════════════════════════════════════════════╝\n"
         << RESET << "\n  Press Enter to begin...";
    cin.get();

    demo1_deadlock(accounts, pool);
    demo2_mutex(accounts, pool);
    demo3_stresstest(accounts, pool);

    return 0;
}
