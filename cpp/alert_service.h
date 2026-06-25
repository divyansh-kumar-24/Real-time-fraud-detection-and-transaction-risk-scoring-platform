#pragma once
#include <string>
#include "transaction.h"

namespace fraudshield {

class AlertService {
public:
    AlertRecord createAlert(const std::string& transaction_id, double risk_score) const;
    std::string levelForScore(double risk_score) const;
};

} // namespace fraudshield
