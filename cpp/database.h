#pragma once
#include <mutex>
#include <string>
#include <sqlite3.h>
#include "transaction.h"

namespace fraudshield {

class Database {
public:
    explicit Database(const std::string& db_path);
    ~Database();

    bool initializeSchema(const std::string& schema_path);
    bool saveTransactionPackage(const TransactionInput& tx,
                                const PredictionSummary& pred,
                                const AlertRecord* alert);

    std::string fetchTransactionJson(const std::string& transaction_id);
    std::string fetchAlertsJson();

private:
    sqlite3* db_;
    std::mutex mutex_;

    bool execute(const std::string& sql);
    std::string currentTimestamp() const;
};

} // namespace fraudshield
