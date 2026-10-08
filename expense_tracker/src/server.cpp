#include "server.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cctype>
#include <algorithm>
#include <vector>

#define BUFFER_SIZE 65536

static std::string getCurrentDate() {
    time_t raw = time(nullptr);
    struct tm* info = localtime(&raw);
    char buf[32];
    if (info) {
        strftime(buf, sizeof(buf), "%Y-%m-%d", info);
        return std::string(buf);
    }
    return "2026-01-01";
}

static bool extractJsonString(const std::string& json, const std::string& key, std::string& out) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return false;

    pos += pattern.length();
    while (pos < json.length() && (std::isspace(json[pos]) || json[pos] == ':')) pos++;

    if (pos < json.length() && json[pos] == '"') {
        pos++;
        std::string result;
        while (pos < json.length() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.length()) {
                pos++;
                if (json[pos] == 'n') result += '\n';
                else if (json[pos] == 'r') result += '\r';
                else if (json[pos] == 't') result += '\t';
                else result += json[pos];
            } else {
                result += json[pos];
            }
            pos++;
        }
        out = result;
        return true;
    }
    return false;
}

static bool extractJsonNumber(const std::string& json, const std::string& key, double& out) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return false;

    pos += pattern.length();
    while (pos < json.length() && (std::isspace(json[pos]) || json[pos] == ':')) pos++;

    if (pos < json.length()) {
        try {
            size_t idx = 0;
            out = std::stod(json.substr(pos), &idx);
            return (idx > 0);
        } catch (...) {
            return false;
        }
    }
    return false;
}

HttpServer::HttpServer(int port, const std::string& webRoot)
    : port(port), webRoot(webRoot), manager("expenses.csv"), serverSocket(INVALID_SOCKET) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
}

HttpServer::~HttpServer() {
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
    }
    WSACleanup();
}

void HttpServer::sendResponse(SOCKET client, int statusCode, const std::string& statusMsg,
                              const std::string& contentType, const std::string& body) {
    std::ostringstream header;
    header << "HTTP/1.1 " << statusCode << " " << statusMsg << "\r\n"
           << "Content-Type: " << contentType << "\r\n"
           << "Content-Length: " << body.length() << "\r\n"
           << "Access-Control-Allow-Origin: *\r\n"
           << "Access-Control-Allow-Methods: GET, POST, OPTIONS, PUT, DELETE\r\n"
           << "Access-Control-Allow-Headers: Content-Type\r\n"
           << "Connection: close\r\n\r\n";

    std::string headerStr = header.str();
    send(client, headerStr.c_str(), static_cast<int>(headerStr.length()), 0);
    if (!body.empty()) {
        send(client, body.c_str(), static_cast<int>(body.length()), 0);
    }
}

void HttpServer::serveFile(SOCKET client, const std::string& filepath, const std::string& contentType) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::string notFound = "{\"error\": \"File not found\"}";
        sendResponse(client, 404, "Not Found", "application/json", notFound);
        return;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    file.close();

    sendResponse(client, 200, "OK", contentType, content);
}

void HttpServer::handleGetExpenses(SOCKET client) {
    std::string json = manager.toJson();
    sendResponse(client, 200, "OK", "application/json; charset=utf-8", json);
}

void HttpServer::handleGetStats(SOCKET client) {
    std::string json = manager.statsToJson();
    sendResponse(client, 200, "OK", "application/json; charset=utf-8", json);
}

void HttpServer::handleAddExpense(SOCKET client, const std::string& body) {
    std::string date, category, description;
    double amount = 0.0;

    extractJsonString(body, "date", date);
    extractJsonString(body, "category", category);
    extractJsonString(body, "description", description);
    extractJsonNumber(body, "amount", amount);

    if (date.empty()) date = getCurrentDate();
    if (category.empty()) category = "Other";

    if (amount <= 0.0) {
        std::string err = "{\"error\": \"Amount must be greater than zero.\"}";
        sendResponse(client, 400, "Bad Request", "application/json", err);
        return;
    }

    int id = manager.addExpense(date, category, amount, description);
    if (id > 0) {
        std::ostringstream resp;
        resp << "{\"success\": true, \"id\": " << id << "}";
        sendResponse(client, 201, "Created", "application/json", resp.str());
    } else {
        std::string err = "{\"error\": \"Failed to add expense.\"}";
        sendResponse(client, 500, "Internal Server Error", "application/json", err);
    }
}

void HttpServer::handleUpdateExpense(SOCKET client, const std::string& body) {
    double idVal = 0.0;
    if (!extractJsonNumber(body, "id", idVal) || idVal <= 0.0) {
        std::string err = "{\"error\": \"Valid expense ID is required.\"}";
        sendResponse(client, 400, "Bad Request", "application/json", err);
        return;
    }
    int id = static_cast<int>(idVal);

    std::string date, category, description;
    double amount = -1.0;

    extractJsonString(body, "date", date);
    extractJsonString(body, "category", category);
    extractJsonString(body, "description", description);
    extractJsonNumber(body, "amount", amount);

    if (manager.updateExpense(id, date, category, amount, description)) {
        std::string resp = "{\"success\": true}";
        sendResponse(client, 200, "OK", "application/json", resp);
    } else {
        std::string err = "{\"error\": \"Expense not found.\"}";
        sendResponse(client, 404, "Not Found", "application/json", err);
    }
}

void HttpServer::handleDeleteExpense(SOCKET client, const std::string& body, const std::string& query) {
    int id = 0;
    double idVal = 0.0;

    if (extractJsonNumber(body, "id", idVal) && idVal > 0.0) {
        id = static_cast<int>(idVal);
    } else if (!query.empty()) {
        size_t pos = query.find("id=");
        if (pos != std::string::npos) {
            try {
                id = std::stoi(query.substr(pos + 3));
            } catch (...) {}
        }
    }

    if (id <= 0) {
        std::string err = "{\"error\": \"Invalid or missing ID.\"}";
        sendResponse(client, 400, "Bad Request", "application/json", err);
        return;
    }

    if (manager.deleteExpense(id)) {
        std::string resp = "{\"success\": true}";
        sendResponse(client, 200, "OK", "application/json", resp);
    } else {
        std::string err = "{\"error\": \"Expense not found.\"}";
        sendResponse(client, 404, "Not Found", "application/json", err);
    }
}

void HttpServer::handleClient(SOCKET client) {
    std::vector<char> buffer(BUFFER_SIZE, 0);
    int bytesReceived = recv(client, buffer.data(), BUFFER_SIZE - 1, 0);
    if (bytesReceived <= 0) {
        closesocket(client);
        return;
    }

    std::string reqStr(buffer.data(), bytesReceived);

    // Acknowledge Expect: 100-continue if present
    if (reqStr.find("100-continue") != std::string::npos || reqStr.find("100-Continue") != std::string::npos) {
        const char* cont = "HTTP/1.1 100 Continue\r\n\r\n";
        send(client, cont, static_cast<int>(strlen(cont)), 0);
    }

    // Parse header and check Content-Length for body completion
    size_t headerEnd = reqStr.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        size_t headerLen = headerEnd + 4;
        size_t bodyReceived = bytesReceived - headerLen;

        int contentLength = 0;
        size_t clPos = reqStr.find("Content-Length:");
        if (clPos == std::string::npos) clPos = reqStr.find("content-length:");
        if (clPos != std::string::npos) {
            try {
                contentLength = std::stoi(reqStr.substr(clPos + 15));
            } catch (...) {}
        }

        while (static_cast<int>(bodyReceived) < contentLength && bytesReceived < BUFFER_SIZE - 1) {
            int r = recv(client, buffer.data() + bytesReceived, (BUFFER_SIZE - 1) - bytesReceived, 0);
            if (r <= 0) break;
            bytesReceived += r;
            bodyReceived += r;
        }
        reqStr.assign(buffer.data(), bytesReceived);
    }

    std::istringstream reqStream(reqStr);
    std::string method, fullUrl, proto;
    reqStream >> method >> fullUrl >> proto;

    std::string path = fullUrl;
    std::string query;
    size_t qPos = fullUrl.find('?');
    if (qPos != std::string::npos) {
        path = fullUrl.substr(0, qPos);
        query = fullUrl.substr(qPos + 1);
    }

    std::string body;
    headerEnd = reqStr.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        body = reqStr.substr(headerEnd + 4);
    }

    // Routing
    if (method == "OPTIONS") {
        sendResponse(client, 204, "No Content", "text/plain", "");
    } else if (method == "GET") {
        if (path == "/" || path == "/index.html") {
            serveFile(client, webRoot + "/index.html", "text/html; charset=utf-8");
        } else if (path == "/style.css") {
            serveFile(client, webRoot + "/style.css", "text/css; charset=utf-8");
        } else if (path == "/app.js") {
            serveFile(client, webRoot + "/app.js", "application/javascript; charset=utf-8");
        } else if (path == "/api/expenses") {
            handleGetExpenses(client);
        } else if (path == "/api/stats") {
            handleGetStats(client);
        } else {
            serveFile(client, webRoot + path, "application/octet-stream");
        }
    } else if (method == "POST") {
        if (path == "/api/expenses") {
            handleAddExpense(client, body);
        } else if (path == "/api/expenses/update") {
            handleUpdateExpense(client, body);
        } else if (path == "/api/expenses/delete") {
            handleDeleteExpense(client, body, query);
        } else {
            std::string err = "{\"error\": \"Endpoint not found\"}";
            sendResponse(client, 404, "Not Found", "application/json", err);
        }
    } else if (method == "DELETE") {
        if (path == "/api/expenses") {
            handleDeleteExpense(client, body, query);
        } else {
            std::string err = "{\"error\": \"Endpoint not found\"}";
            sendResponse(client, 404, "Not Found", "application/json", err);
        }
    } else {
        std::string err = "{\"error\": \"Method not allowed\"}";
        sendResponse(client, 405, "Method Not Allowed", "application/json", err);
    }

    closesocket(client);
}

void HttpServer::start() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create server socket.\n";
        return;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed on port " << port << ".\n";
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
        return;
    }

    listen(serverSocket, 10);

    std::cout << "\n=================================================================\n";
    std::cout << "   EXPENSE TRACKER WEB SERVER RUNNING (MODERN C++ BACKEND)       \n";
    std::cout << "=================================================================\n";
    std::cout << "   URL: http://localhost:" << port << "\n";
    std::cout << "   Database: expenses.csv (" << manager.getCount() << " records loaded)\n";
    std::cout << "   Press Ctrl+C in this terminal to stop the server.\n";
    std::cout << "=================================================================\n\n";

    while (true) {
        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        SOCKET clientSock = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientSock == INVALID_SOCKET) {
            continue;
        }

        handleClient(clientSock);
    }
}
