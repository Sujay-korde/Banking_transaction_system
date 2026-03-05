#include "DatabaseManager.h"
#include <sstream>

DatabaseManager::DatabaseManager() : conn(nullptr), connected(false) {
    conn = mysql_init(nullptr);
    if (!conn) throw std::runtime_error("[DB] mysql_init() failed");
}

DatabaseManager::~DatabaseManager() {
    disconnect();
}

bool DatabaseManager::connect(const std::string& host, const std::string& user,
                               const std::string& password, const std::string& database,
                               int port) {
    if (!mysql_real_connect(conn, host.c_str(), user.c_str(),
                            password.c_str(), database.c_str(),
                            port, nullptr, 0)) {
        std::cerr << "[DB] Connection failed: " << mysql_error(conn) << "\n";
        return false;
    }
    connected = true;
    // Auto-reconnect on dropped connections
    bool reconnect = true;
    mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);
    return true;
}

void DatabaseManager::disconnect() {
    if (conn) {
        mysql_close(conn);
        conn = nullptr;
        connected = false;
    }
}

void DatabaseManager::executeQuery(const std::string& query) {
    if (mysql_query(conn, query.c_str()))
        throw std::runtime_error(std::string("[DB] Query failed: ") + mysql_error(conn));
}

bool DatabaseManager::accountExists(const std::string& account_number) {
    std::string q = "SELECT COUNT(*) FROM accounts WHERE account_number='"
                    + account_number + "'";
    executeQuery(q);
    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);
    int count = row ? std::stoi(row[0]) : 0;
    mysql_free_result(res);
    return count > 0;
}

bool DatabaseManager::createAccount(const std::string& account_number,
                                     const std::string& type, double balance) {
    std::ostringstream q;
    q << "INSERT INTO accounts (account_number, account_type, balance) VALUES ('"
      << account_number << "','" << type << "'," << balance << ")";
    try {
        executeQuery(q.str());
        return true;
    } catch (...) { return false; }
}

double DatabaseManager::getBalance(const std::string& account_number) {
    std::string q = "SELECT balance FROM accounts WHERE account_number='"
                    + account_number + "'";
    executeQuery(q);
    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);
    double bal = row ? std::stod(row[0]) : -1.0;
    mysql_free_result(res);
    return bal;
}

bool DatabaseManager::updateBalance(const std::string& account_number, double new_balance) {
    std::ostringstream q;
    q << "UPDATE accounts SET balance=" << new_balance
      << " WHERE account_number='" << account_number << "'";
    try {
        executeQuery(q.str());
        return mysql_affected_rows(conn) > 0;
    } catch (...) { return false; }
}

long long DatabaseManager::logTransaction(const std::string& from_acc,
                                           const std::string& to_acc,
                                           const std::string& type,
                                           double amount,
                                           const std::string& status,
                                           int thread_id,
                                           int processing_ms) {
    std::ostringstream q;
    q << "INSERT INTO transactions "
      << "(from_account, to_account, transaction_type, amount, status, thread_id, processing_time_ms) "
      << "VALUES ('"
      << (from_acc.empty() ? "NULL" : from_acc) << "','"
      << (to_acc.empty()   ? "NULL" : to_acc)   << "','"
      << type   << "',"
      << amount << ",'"
      << status << "',"
      << thread_id << ","
      << processing_ms << ")";
    try {
        executeQuery(q.str());
        return (long long)mysql_insert_id(conn);
    } catch (...) { return -1; }
}

void DatabaseManager::logEvent(long long transaction_id, const std::string& event) {
    std::ostringstream q;
    q << "INSERT INTO transaction_log (transaction_id, event) VALUES ("
      << transaction_id << ",'" << event << "')";
    try { executeQuery(q.str()); } catch (...) {}
}
