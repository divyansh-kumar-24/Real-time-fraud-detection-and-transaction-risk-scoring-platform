#pragma once
#include <string>
#include "transaction.h"

namespace fraudshield {

class PredictionService {
public:
    PredictionService(std::string models_dir, std::string script_path);
    std::string score(const TransactionInput& tx) const;

private:
    std::string models_dir_;
    std::string script_path_;
};

} // namespace fraudshield
