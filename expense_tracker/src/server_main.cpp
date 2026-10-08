#include "server.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    int port = 8080;
    std::string webRoot = "web";

    if (argc > 1) {
        int p = std::atoi(argv[1]);
        if (p > 0 && p <= 65535) port = p;
    }

    if (argc > 2) {
        webRoot = argv[2];
    }

    std::cout << "Starting C++ Expense Tracker Web Server...\n";
    HttpServer server(port, webRoot);
    server.start();

    return 0;
}
