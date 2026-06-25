#include "alert_service.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace fraudshield {

std::string AlertService::levelForScore(double risk_score) const {
    if (risk_score >= 80.0) return "HIGH";
    if (risk_score >= 50.0) return "MEDIUM";
    return "LOW";
}

AlertRecord AlertService::createAlert(const std::string& transaction_id, double risk_score) const {
    AlertRecord alert;
    alert.transaction_id = transaction_id;
    alert.alert_level = levelForScore(risk_score);

    if (alert.alert_level == "HIGH") {
        alert.message = "High-risk transaction flagged for immediate review.";
    } else if (alert.alert_level == "MEDIUM") {
        alert.message = "Medium-risk transaction flagged for analyst review.";
    } else {
        alert.message = "Low-risk transaction recorded without escalation.";
    }
    return alert;
}

} // namespace fraudshield
