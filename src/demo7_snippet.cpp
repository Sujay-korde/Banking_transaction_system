// ─────────────────────────────────────────────────────────────
//  DEMO 7 — Readers-Writers Problem (Week 4)
// ─────────────────────────────────────────────────────────────
void demoReadersWriters(std::vector<std::shared_ptr<Account>>& accounts) {
    printSection("DEMO 7: READERS-WRITERS PROBLEM  (Week 4)");

    std::cout << "\n  Scenario: Balance inquiry desk at peak hour\n";
    std::cout << "  READERS : Multiple threads check balances simultaneously\n";
    std::cout << "  WRITERS : Transfer thread needs exclusive access\n\n";
    std::cout << "  Rule: Many readers OR one writer — never both\n\n";

    ReadWriteLock rwl;
    std::mutex print_m;
    std::atomic<bool> done{false};

    // Shared data — account balances snapshot
    std::vector<double> balances(accounts.size());
    for (int i = 0; i < (int)accounts.size(); i++)
        balances[i] = accounts[i]->getBalance();

    // ── 6 Reader threads — check balances simultaneously ──
    std::vector<std::thread> readers;
    for (int r = 0; r < 6; r++) {
        readers.emplace_back([&, r]() {
            for (int iter = 0; iter < 3; iter++) {
                {
                    std::lock_guard<std::mutex> lk(print_m);
                    std::cout << CYAN << "  [Reader-" << r
                              << "] Acquiring read lock...\n" << RESET;
                }

                ReadGuard rg(rwl);  // OS: readLock()

                {
                    std::lock_guard<std::mutex> lk(print_m);
                    std::cout << GREEN << "  [Reader-" << r
                              << "] Reading balances (shared access OK)\n" << RESET;
                }

                // Simulate reading — multiple readers run simultaneously
                std::this_thread::sleep_for(std::chrono::milliseconds(150));

                double total = 0;
                for (auto b : balances) total += b;

                {
                    std::lock_guard<std::mutex> lk(print_m);
                    std::cout << "  [Reader-" << r << "] Total balance: $"
                              << std::fixed << std::setprecision(2) << total
                              << " — releasing read lock\n";
                }
                // ReadGuard destructor calls readUnlock()

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });
    }

    // ── 2 Writer threads — transfer (exclusive access) ──
    std::vector<std::thread> writers;
    for (int w = 0; w < 2; w++) {
        writers.emplace_back([&, w]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100 + w * 200));

            {
                std::lock_guard<std::mutex> lk(print_m);
                std::cout << YELLOW << "\n  [Writer-" << w
                          << "] Requesting WRITE lock (transfer)...\n" << RESET;
            }

            WriteGuard wg(rwl);  // OS: writeLock() — blocks until all readers done

            {
                std::lock_guard<std::mutex> lk(print_m);
                std::cout << RED << "  [Writer-" << w
                          << "] Got EXCLUSIVE access — transferring $500\n" << RESET;
                std::cout << "  [Writer-" << w
                          << "] All readers BLOCKED during this transfer\n";
            }

            // Simulate transfer
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            // Update balance snapshot
            balances[w] -= 500.0;
            balances[(w+1) % balances.size()] += 500.0;

            {
                std::lock_guard<std::mutex> lk(print_m);
                std::cout << GREEN << "  [Writer-" << w
                          << "] Transfer done — releasing write lock\n\n" << RESET;
            }
            // WriteGuard destructor calls writeUnlock()
        });
    }

    for (auto& r : readers) r.join();
    for (auto& w : writers) w.join();

    rwl.printStats();

    std::cout << GREEN << "\n  ✓ Readers-Writers complete!\n" << RESET;
    std::cout << "  Multiple readers ran simultaneously\n";
    std::cout << "  Writers got exclusive access — zero data corruption\n";

    std::cout << "\n  Press Enter to continue...";
    std::cin.get();
}
