#include "database.h"
#include "json_utils.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace fraudshield {

Database::Database(const std::string& db_path) : db_(nullptr) {
    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
        std::string err = db_ ? sqlite3_errmsg(db_) : "unknown error";
        throw std::runtime_error("Failed to open SQLite database: " + err);
    }
    execute("PRAGMA foreign_keys = ON;");
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::execute(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Database::initializeSchema(const std::string& schema_path) {
    std::string sql = readFile(schema_path);
    return execute(sql);
}

std::string Database::currentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream os;
    os << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return os.str();
}

bool Database::saveTransactionPackage(const TransactionInput& tx,
                                      const PredictionSummary& pred,
                                      const AlertRecord* alert) {
    std::lock_guard<std::mutex> lock(mutex_);

    char* errMsg = nullptr;
    if (sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }

    auto rollback = [&]() {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    };

    sqlite3_stmt* stmt = nullptr;

    const std::string txJson = transactionToJson(tx);
    const std::string predJson = pred.raw_json.empty() ? "{}" : pred.raw_json;
    const std::string modelProbJson = "{}";  // stored in raw prediction json
    const std::string explJson = "{}";       // stored in raw prediction json

    const std::string insertTransactionSql =
        "INSERT OR REPLACE INTO transactions(transaction_id, request_json, created_at) VALUES (?, ?, ?);";
    if (sqlite3_prepare_v2(db_, insertTransactionSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        rollback();
        return false;
    }
    sqlite3_bind_text(stmt, 1, tx.transaction_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, txJson.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, currentTimestamp().c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        rollback();
        return false;
    }
    sqlite3_finalize(stmt);

    const std::string insertPredSql =
        "INSERT OR REPLACE INTO predictions(transaction_id, fraud_probability, risk_score, alert_level, "
        "model_probabilities_json, explanations_json, prediction_json, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db_, insertPredSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        rollback();
        return false;
    }
    sqlite3_bind_text(stmt, 1, tx.transaction_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, pred.fraud_probability);
    sqlite3_bind_double(stmt, 3, pred.risk_score);
    sqlite3_bind_text(stmt, 4, pred.alert_level.c_str(), -1, SQLITE_TRANSIENT);

    // Keep these as placeholders because the full model JSON is already in prediction_json.
    sqlite3_bind_text(stmt, 5, "{}", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, "{}", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, predJson.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, currentTimestamp().c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        rollback();
        return false;
    }
    sqlite3_finalize(stmt);

    if (alert && !alert->alert_level.empty()) {
        const std::string insertAlertSql =
            "INSERT INTO alerts(transaction_id, alert_level, message, created_at) VALUES (?, ?, ?, ?);";
        if (sqlite3_prepare_v2(db_, insertAlertSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            rollback();
            return false;
        }
        sqlite3_bind_text(stmt, 1, alert->transaction_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, alert->alert_level.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, alert->message.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, currentTimestamp().c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            rollback();
            return false;
        }
        sqlite3_finalize(stmt);
    }

    if (sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        rollback();
        return false;
    }

    return true;
}

std::string Database::fetchTransactionJson(const std::string& transaction_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql =
        "SELECT t.request_json, p.prediction_json, p.fraud_probability, p.risk_score, p.alert_level "
        "FROM transactions t LEFT JOIN predictions p ON t.transaction_id = p.transaction_id "
        "WHERE t.transaction_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return R"({"error":"failed to prepare query"})";
    }
    sqlite3_bind_text(stmt, 1, transaction_id.c_str(), -1, SQLITE_TRANSIENT);

    std::ostringstream os;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* requestJson = sqlite3_column_text(stmt, 0);
        const unsigned char* predJson = sqlite3_column_text(stmt, 1);
        double prob = sqlite3_column_double(stmt, 2);
        double score = sqlite3_column_double(stmt, 3);
        const unsigned char* level = sqlite3_column_text(stmt, 4);

        os << "{";
        os << "\"transaction_id\":\"" << jsonEscape(transaction_id) << "\",";
        os << "\"request_json\":" << (requestJson ? "\"" + jsonEscape(reinterpret_cast<const char*>(requestJson)) + "\"" : "null") << ",";
        os << "\"prediction_json\":" << (predJson ? "\"" + jsonEscape(reinterpret_cast<const char*>(predJson)) + "\"" : "null") << ",";
        os << "\"fraud_probability\":" << prob << ",";
        os << "\"risk_score\":" << score << ",";
        os << "\"alert_level\":\"" << (level ? jsonEscape(reinterpret_cast<const char*>(level)) : "") << "\"";
        os << "}";
    } else {
        os << R"({"error":"transaction not found"})";
    }
    sqlite3_finalize(stmt);
    return os.str();
}

std::string Database::fetchAlertsJson() {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "SELECT transaction_id, alert_level, message, created_at FROM alerts ORDER BY alert_id DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return R"({"error":"failed to prepare query"})";
    }

    std::ostringstream os;
    os << "{\"alerts\":[";
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) os << ",";
        first = false;
        const char* transaction_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* level = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

        os << "{";
        os << "\"transaction_id\":\"" << jsonEscape(transaction_id ? transaction_id : "") << "\",";
        os << "\"alert_level\":\"" << jsonEscape(level ? level : "") << "\",";
        os << "\"message\":\"" << jsonEscape(message ? message : "") << "\",";
        os << "\"created_at\":\"" << jsonEscape(created_at ? created_at : "") << "\"";
        os << "}";
    }
    os << "]}";
    sqlite3_finalize(stmt);
    return os.str();
}

} // namespace fraudshield
