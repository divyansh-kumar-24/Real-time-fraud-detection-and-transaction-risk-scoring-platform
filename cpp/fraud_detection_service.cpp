#include "fraud_detection_service.h"
#include "json_utils.h"
#include <sstream>
#include <stdexcept>

namespace fraudshield {

FraudDetectionService::FraudDetectionService(const std::string& db_path,
                                             const std::string& schema_path,
                                             const std::string& models_dir,
                                             const std::string& predict_script)
    : database_(db_path), predictor_(models_dir, predict_script) {
    if (!database_.initializeSchema(schema_path)) {
        throw std::runtime_error("Failed to initialize SQLite schema.");
    }
}

std::string FraudDetectionService::health() const {
    return R"({"status":"ok","service":"Fraud Detection Platform"})";
}

std::string FraudDetectionService::processTransaction(const std::string& request_body) {
    TransactionInput tx = parseTransactionInput(request_body);
    if (tx.transaction_id.empty()) {
        tx.transaction_id = "txn_" + std::to_string(std::rand());
    }

    std::string predictionJson = predictor_.score(tx);

    double fraudProbability = 0.0;
    double riskScore = 0.0;
    std::string alertLevel;

    if (!parsePredictionSummary(predictionJson, fraudProbability, riskScore, alertLevel)) {
        return R"({"error":"prediction parsing failed"})";
    }

    PredictionSummary summary;
    summary.transaction_id = tx.transaction_id;
    summary.fraud_probability = fraudProbability;
    summary.risk_score = riskScore;
    summary.alert_level = alertLevel;
    summary.raw_json = predictionJson;

    AlertRecord alert = alert_service_.createAlert(tx.transaction_id, riskScore);
    const AlertRecord* alertPtr = (alert.alert_level == "LOW") ? nullptr : &alert;

    if (!database_.saveTransactionPackage(tx, summary, alertPtr)) {
        return R"({"error":"failed to persist transaction"})";
    }

    // Add a small marker that backend persistence succeeded.
    std::ostringstream os;
    os << "{";
    os << "\"status\":\"success\",";
    os << "\"persisted\":true,";
    os << "\"prediction\":" << predictionJson;
    os << "}";
    return os.str();
}

std::string FraudDetectionService::getTransaction(const std::string& transaction_id) {
    return database_.fetchTransactionJson(transaction_id);
}

std::string FraudDetectionService::getAlerts() {
    return database_.fetchAlertsJson();
}

} // namespace fraudshield
