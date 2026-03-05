#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <mysql/mysql.h>
#include <string>
#include <iostream>
#include <stdexcept>

/*
 * DatabaseManager
 * Handles a single MySQL connection + all query operations.
 * One instance = one connection in the pool.
 */
class DatabaseManager {
private:
    MYSQL* conn;
    bool connected;

public:
    DatabaseManager();
    ~DatabaseManager();

    bool connect(const std::string& host, const std::string& user,
                 const std::string& password, const std::string& database,
                 int port = 3306);

    void disconnect();
    bool isConnected() const { return connected; }
    MYSQL* getConnection() { return conn; }

    // Account operations
    bool accountExists(const std::string& account_number);
    bool createAccount(const std::string& account_number,
                       const std::string& type, double balance);
    double getBalance(const std::string& account_number);
    bool updateBalance(const std::string& account_number, double new_balance);

    // Transaction logging
    long long logTransaction(const std::string& from_acc,
                             const std::string& to_acc,
                             const std::string& type,
                             double amount,
                             const std::string& status,
                             int thread_id,
                             int processing_ms);

    void logEvent(long long transaction_id, const std::string& event);

    // Query helpers
    void executeQuery(const std::string& query);
};

#endif
