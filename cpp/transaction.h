#pragma once
#include <array>
#include <map>
#include <string>
#include <vector>

namespace fraudshield {

struct TransactionInput {
    std::string transaction_id;
    std::string merchant;
    std::string timestamp;
    double Time = 0.0;
    std::array<double, 28> V{};
    double Amount = 0.0;
};

struct PredictionSummary {
    std::string transaction_id;
    double fraud_probability = 0.0;
    double risk_score = 0.0;
    std::string alert_level;
    std::string raw_json;
};

struct AlertRecord {
    std::string transaction_id;
    std::string alert_level;
    std::string message;
    std::string created_at;
};

} // namespace fraudshield
