#include <stdio.h>
#include <stdlib.h>
#include "server.h"

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    const char *web_root = DEFAULT_WEB_ROOT;

    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            port = DEFAULT_PORT;
        }
    }

    if (argc > 2) {
        web_root = argv[2];
    }

    printf("Starting Expense Tracker Web Service...\n");
    return start_http_server(port, web_root);
}
