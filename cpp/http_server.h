#pragma once
#include <string>
#include "fraud_detection_service.h"

namespace fraudshield {

class HttpServer {
public:
    HttpServer(int port, FraudDetectionService& service);
    void run();

private:
    int port_;
    FraudDetectionService& service_;

    std::string makeResponse(const std::string& status,
                             const std::string& contentType,
                             const std::string& body) const;
    void handleClient(int clientFd);
};

} // namespace fraudshield
