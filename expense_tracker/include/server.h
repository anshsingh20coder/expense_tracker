#ifndef SERVER_H
#define SERVER_H

#include "expense.h"

#define DEFAULT_PORT 8080
#define DEFAULT_WEB_ROOT "web"

/**
 * Initializes and starts the embedded C HTTP server on the specified port.
 * Serves static assets from web_root and responds to REST API requests.
 */
int start_http_server(int port, const char *web_root);

#endif /* SERVER_H */
