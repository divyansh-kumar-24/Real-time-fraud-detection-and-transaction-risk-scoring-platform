#pragma once
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "transaction.h"

namespace fraudshield {

inline std::string readFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("Unable to open file: " + path);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

inline void writeFile(const std::string& path, const std::string& content) {
    std::ofstream out(path);
    if (!out.is_open()) {
        throw std::runtime_error("Unable to write file: " + path);
    }
    out << content;
}

inline std::string jsonEscape(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        switch (c) {
            case '\\': o << "\\\\"; break;
            case '"':  o << "\\\""; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    o << "\\u"
                      << std::hex << std::setw(4) << std::setfill('0')
                      << (int)c;
                } else {
                    o << c;
                }
        }
    }
    return o.str();
}

inline bool extractStringField(const std::string& body, const std::string& key, std::string& out) {
    std::regex rgx("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch m;
    if (std::regex_search(body, m, rgx)) {
        out = m[1];
        return true;
    }
    return false;
}

inline bool extractNumberField(const std::string& body, const std::string& key, double& out) {
    std::regex rgx("\\\"" + key + "\\\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?)");
    std::smatch m;
    if (std::regex_search(body, m, rgx)) {
        out = std::stod(m[1]);
        return true;
    }
    return false;
}

inline TransactionInput parseTransactionInput(const std::string& body) {
    TransactionInput tx;
    extractStringField(body, "transaction_id", tx.transaction_id);
    extractStringField(body, "merchant", tx.merchant);
    extractStringField(body, "timestamp", tx.timestamp);

    extractNumberField(body, "Time", tx.Time);
    extractNumberField(body, "Amount", tx.Amount);

    for (int i = 1; i <= 28; ++i) {
        double v = 0.0;
        extractNumberField(body, "V" + std::to_string(i), v);
        tx.V[i - 1] = v;
    }
    return tx;
}

inline std::string transactionToJson(const TransactionInput& tx) {
    std::ostringstream os;
    os << "{";
    os << "\"transaction_id\":\"" << jsonEscape(tx.transaction_id) << "\",";
    os << "\"merchant\":\"" << jsonEscape(tx.merchant) << "\",";
    os << "\"timestamp\":\"" << jsonEscape(tx.timestamp) << "\",";
    os << "\"Time\":" << tx.Time << ",";
    for (int i = 0; i < 28; ++i) {
        os << "\"V" << (i + 1) << "\":" << tx.V[i] << ",";
    }
    os << "\"Amount\":" << tx.Amount;
    os << "}";
    return os.str();
}

inline std::string toJsonArray(const std::vector<std::string>& items) {
    std::ostringstream os;
    os << "[";
    for (size_t i = 0; i < items.size(); ++i) {
        if (i) os << ",";
        os << "\"" << jsonEscape(items[i]) << "\"";
    }
    os << "]";
    return os.str();
}

inline std::string toJsonDoubleArray(const std::vector<double>& items) {
    std::ostringstream os;
    os << "[";
    for (size_t i = 0; i < items.size(); ++i) {
        if (i) os << ",";
        os << items[i];
    }
    os << "]";
    return os.str();
}

inline bool parsePredictionSummary(const std::string& json, double& fraud_probability, double& risk_score, std::string& alert_level) {
    std::regex probRgx("\\\"fraud_probability\\\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?)");
    std::regex riskRgx("\\\"risk_score\\\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?)");
    std::regex levelRgx("\\\"alert_level\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch m;

    if (std::regex_search(json, m, probRgx)) fraud_probability = std::stod(m[1]); else return false;
    if (std::regex_search(json, m, riskRgx)) risk_score = std::stod(m[1]); else return false;
    if (std::regex_search(json, m, levelRgx)) alert_level = m[1]; else return false;

    return true;
}

} // namespace fraudshield
