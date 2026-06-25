#pragma once
#include <string>
#include "alert_service.h"
#include "database.h"
#include "prediction_service.h"
#include "transaction.h"

namespace fraudshield {

class FraudDetectionService {
public:
    FraudDetectionService(const std::string& db_path,
                          const std::string& schema_path,
                          const std::string& models_dir,
                          const std::string& predict_script);

    std::string health() const;
    std::string processTransaction(const std::string& request_body);
    std::string getTransaction(const std::string& transaction_id);
    std::string getAlerts();

private:
    Database database_;
    PredictionService predictor_;
    AlertService alert_service_;
};

} // namespace fraudshield
