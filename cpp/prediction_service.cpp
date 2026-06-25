#include "prediction_service.h"
#include "json_utils.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdio>
#include <unistd.h>

namespace fraudshield {

PredictionService::PredictionService(std::string models_dir, std::string script_path)
    : models_dir_(std::move(models_dir)), script_path_(std::move(script_path)) {}

static std::string makeTempPath(const std::string& suffix) {
    char tmpl[] = "/tmp/fraudshield_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd == -1) {
        throw std::runtime_error("Failed to create temp file");
    }
    close(fd);
    std::string path = tmpl;
    if (!suffix.empty()) path += suffix;
    // rename the created file with suffix if needed
    if (!suffix.empty()) {
        std::string finalPath = std::string(tmpl) + suffix;
        std::rename(path.c_str(), finalPath.c_str());
        return finalPath;
    }
    return path;
}

std::string PredictionService::score(const TransactionInput& tx) const {
    const std::string inputPath = makeTempPath(".json");
    const std::string outputPath = makeTempPath("_out.json");

    try {
        writeFile(inputPath, transactionToJson(tx));
    } catch (...) {
        throw;
    }

    std::ostringstream cmd;
    cmd << "python3 \"" << script_path_ << "\""
        << " --input \"" << inputPath << "\""
        << " --output \"" << outputPath << "\""
        << " --models \"" << models_dir_ << "\"";

    int rc = std::system(cmd.str().c_str());

    std::string result;
    if (rc == 0) {
        result = readFile(outputPath);
    } else {
        result = R"({"error":"prediction script failed"})";
    }

    std::remove(inputPath.c_str());
    std::remove(outputPath.c_str());
    return result;
}

} // namespace fraudshield
