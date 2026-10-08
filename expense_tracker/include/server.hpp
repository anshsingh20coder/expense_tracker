#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <map>
#include <winsock2.h>
#include "expense_manager.hpp"

struct HttpRequest {
    std::string method;
    std::string path;
    std::string query;
    std::string body;
    std::map<std::string, std::string> headers;
};

class HttpServer {
private:
    int port;
    std::string webRoot;
    ExpenseManager manager;
    SOCKET serverSocket;

    void handleClient(SOCKET client);
    void sendResponse(SOCKET client, int statusCode, const std::string& statusMsg,
                      const std::string& contentType, const std::string& body);
    void serveFile(SOCKET client, const std::string& filepath, const std::string& contentType);

    void handleGetExpenses(SOCKET client);
    void handleGetStats(SOCKET client);
    void handleAddExpense(SOCKET client, const std::string& body);
    void handleUpdateExpense(SOCKET client, const std::string& body);
    void handleDeleteExpense(SOCKET client, const std::string& body, const std::string& query);

public:
    HttpServer(int port = 8080, const std::string& webRoot = "web");
    ~HttpServer();

    void start();
};

#endif // SERVER_HPP
