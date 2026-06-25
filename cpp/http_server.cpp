#include "http_server.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

namespace fraudshield {

HttpServer::HttpServer(int port, FraudDetectionService& service)
    : port_(port), service_(service) {}

std::string HttpServer::makeResponse(const std::string& status,
                                     const std::string& contentType,
                                     const std::string& body) const {
    std::ostringstream os;
    os << "HTTP/1.1 " << status << "\r\n";
    os << "Content-Type: " << contentType << "\r\n";
    os << "Content-Length: " << body.size() << "\r\n";
    os << "Connection: close\r\n\r\n";
    os << body;
    return os.str();
}

void HttpServer::handleClient(int clientFd) {
    std::string request;
    char buffer[8192];

    while (true) {
        ssize_t n = recv(clientFd, buffer, sizeof(buffer), 0);
        if (n <= 0) break;
        request.append(buffer, buffer + n);
        if (request.find("\r\n\r\n") != std::string::npos) break;
    }

    if (request.empty()) {
        close(clientFd);
        return;
    }

    auto headerEnd = request.find("\r\n\r\n");
    std::string headers = request.substr(0, headerEnd);
    std::string body = request.substr(headerEnd + 4);

    std::istringstream hs(headers);
    std::string method, path, version;
    hs >> method >> path >> version;

    size_t contentLength = 0;
    std::string line;
    std::getline(hs, line); // consume rest of first line
    while (std::getline(hs, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto pos = line.find(":");
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            while (!value.empty() && value.front() == ' ') value.erase(value.begin());
            if (key == "Content-Length") {
                contentLength = static_cast<size_t>(std::stoul(value));
            }
        }
    }

    while (body.size() < contentLength) {
        ssize_t n = recv(clientFd, buffer, sizeof(buffer), 0);
        if (n <= 0) break;
        body.append(buffer, buffer + n);
    }

    std::string responseBody;
    std::string status = "200 OK";

    try {
        if (method == "GET" && path == "/health") {
            responseBody = service_.health();
        } else if (method == "POST" && path == "/transactions") {
            responseBody = service_.processTransaction(body);
        } else if (method == "GET" && path.rfind("/transactions/", 0) == 0) {
            auto id = path.substr(std::string("/transactions/").size());
            responseBody = service_.getTransaction(id);
        } else if (method == "GET" && path == "/alerts") {
            responseBody = service_.getAlerts();
        } else {
            status = "404 Not Found";
            responseBody = R"({"error":"endpoint not found"})";
        }
    } catch (const std::exception& e) {
        status = "500 Internal Server Error";
        responseBody = std::string("{\"error\":\"") + e.what() + "\"}";
    }

    std::string response = makeResponse(status, "application/json", responseBody);
    send(clientFd, response.c_str(), response.size(), 0);
    close(clientFd);
}

void HttpServer::run() {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(serverFd);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverFd, 10) < 0) {
        close(serverFd);
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "FraudShield HTTP server running on port " << port_ << std::endl;

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientFd < 0) continue;

        std::thread(&HttpServer::handleClient, this, clientFd).detach();
    }
}

} // namespace fraudshield
