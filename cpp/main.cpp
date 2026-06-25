#include "http_server.h"
#include <cstdlib>
#include <iostream>

using namespace fraudshield;

int main(int argc, char** argv) {
    try {
        std::string dbPath = "fraudshield.db";
        std::string schemaPath = "sql/schema.sql";
        std::string modelsDir = "models";
        std::string predictScript = "scripts/predict.py";
        int port = 8080;

        if (argc > 1) port = std::atoi(argv[1]);

        FraudDetectionService service(dbPath, schemaPath, modelsDir, predictScript);
        HttpServer server(port, service);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
