#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "server.h"
#include "expense.h"
#include "utils.h"

#define BUFFER_SIZE 65536

static ExpenseList g_expense_list;

static void json_escape(const char *src, char *dest, size_t dest_size) {
    size_t d = 0;
    for (size_t s = 0; src[s] != '\0' && d < dest_size - 2; s++) {
        if (src[s] == '"' || src[s] == '\\') {
            if (d < dest_size - 3) {
                dest[d++] = '\\';
                dest[d++] = src[s];
            }
        } else if (src[s] == '\n') {
            if (d < dest_size - 3) { dest[d++] = '\\'; dest[d++] = 'n'; }
        } else if (src[s] == '\r') {
            if (d < dest_size - 3) { dest[d++] = '\\'; dest[d++] = 'r'; }
        } else {
            dest[d++] = src[s];
        }
    }
    dest[d] = '\0';
}

static int extract_json_string(const char *json, const char *key, char *out, size_t out_size) {
    if (!json || !key || !out || out_size == 0) return 0;
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p) return 0;
    p += strlen(pattern);
    while (*p && (isspace((unsigned char)*p) || *p == ':')) p++;
    if (*p == '\"') {
        p++;
        size_t idx = 0;
        while (*p && *p != '\"' && idx < out_size - 1) {
            if (*p == '\\' && *(p + 1)) {
                p++;
                if (*p == 'n') out[idx++] = '\n';
                else if (*p == 'r') out[idx++] = '\r';
                else if (*p == 't') out[idx++] = '\t';
                else out[idx++] = *p;
                p++;
            } else {
                out[idx++] = *p++;
            }
        }
        out[idx] = '\0';
        return 1;
    }
    return 0;
}

static int extract_json_number(const char *json, const char *key, double *out_val) {
    if (!json || !key) return 0;
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p) return 0;
    p += strlen(pattern);
    while (*p && (isspace((unsigned char)*p) || *p == ':')) p++;
    if (*p) {
        char *endptr;
        double val = strtod(p, &endptr);
        if (endptr != p) {
            *out_val = val;
            return 1;
        }
    }
    return 0;
}

static void send_response(SOCKET client, int status_code, const char *status_msg,
                          const char *content_type, const char *body, size_t body_len) {
    char header[1024];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %lu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS, PUT, DELETE\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "\r\n",
        status_code, status_msg, content_type, (unsigned long)body_len);

    send(client, header, header_len, 0);
    if (body && body_len > 0) {
        send(client, body, (int)body_len, 0);
    }
}

static void serve_file(SOCKET client, const char *filepath, const char *content_type) {
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        const char *not_found = "{\"error\": \"File not found\"}";
        send_response(client, 404, "Not Found", "application/json", not_found, strlen(not_found));
        return;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size < 0) {
        fclose(f);
        const char *err = "{\"error\": \"File read error\"}";
        send_response(client, 500, "Internal Server Error", "application/json", err, strlen(err));
        return;
    }

    char *buf = (char *)malloc(file_size);
    if (!buf) {
        fclose(f);
        const char *err = "{\"error\": \"Memory allocation failed\"}";
        send_response(client, 500, "Internal Server Error", "application/json", err, strlen(err));
        return;
    }

    size_t read_bytes = fread(buf, 1, file_size, f);
    fclose(f);

    send_response(client, 200, "OK", content_type, buf, read_bytes);
    free(buf);
}

static void handle_get_expenses(SOCKET client) {
    // Dynamic JSON generation
    size_t est_size = 128 + g_expense_list.count * 256;
    char *json = (char *)malloc(est_size);
    if (!json) {
        const char *err = "[]";
        send_response(client, 500, "Internal Server Error", "application/json", err, strlen(err));
        return;
    }

    size_t offset = 0;
    offset += snprintf(json + offset, est_size - offset, "[\n");

    char esc_cat[MAX_CAT_LEN * 2];
    char esc_desc[MAX_DESC_LEN * 2];

    for (size_t i = 0; i < g_expense_list.count; i++) {
        const Expense *e = &g_expense_list.items[i];
        json_escape(e->category, esc_cat, sizeof(esc_cat));
        json_escape(e->description, esc_desc, sizeof(esc_desc));

        offset += snprintf(json + offset, est_size - offset,
            "  {\"id\": %d, \"date\": \"%s\", \"category\": \"%s\", \"amount\": %.2f, \"description\": \"%s\"}%s\n",
            e->id, e->date, esc_cat, e->amount, esc_desc,
            (i + 1 < g_expense_list.count) ? "," : "");
    }

    offset += snprintf(json + offset, est_size - offset, "]");
    send_response(client, 200, "OK", "application/json", json, offset);
    free(json);
}

typedef struct {
    char name[MAX_CAT_LEN];
    double total;
    size_t count;
} CatStat;

static void handle_get_stats(SOCKET client) {
    double grand_total = 0.0;
    size_t count = g_expense_list.count;

    CatStat cats[64];
    size_t cat_count = 0;

    const Expense *highest = (count > 0) ? &g_expense_list.items[0] : NULL;
    const Expense *lowest = (count > 0) ? &g_expense_list.items[0] : NULL;

    for (size_t i = 0; i < count; i++) {
        const Expense *e = &g_expense_list.items[i];
        grand_total += e->amount;

        if (highest && e->amount > highest->amount) highest = e;
        if (lowest && e->amount < lowest->amount) lowest = e;

        int found = 0;
        for (size_t c = 0; c < cat_count; c++) {
            if (strcmp(cats[c].name, e->category) == 0) {
                cats[c].total += e->amount;
                cats[c].count++;
                found = 1;
                break;
            }
        }
        if (!found && cat_count < 64) {
            strncpy(cats[cat_count].name, e->category, MAX_CAT_LEN - 1);
            cats[cat_count].name[MAX_CAT_LEN - 1] = '\0';
            cats[cat_count].total = e->amount;
            cats[cat_count].count = 1;
            cat_count++;
        }
    }

    double average = (count > 0) ? (grand_total / (double)count) : 0.0;

    size_t json_size = 4096 + cat_count * 256;
    char *json = (char *)malloc(json_size);
    if (!json) {
        const char *err = "{}";
        send_response(client, 500, "Internal Server Error", "application/json", err, strlen(err));
        return;
    }

    size_t offset = 0;
    offset += snprintf(json + offset, json_size - offset,
        "{\n"
        "  \"count\": %lu,\n"
        "  \"total\": %.2f,\n"
        "  \"average\": %.2f,\n"
        "  \"categories\": [\n",
        (unsigned long)count, grand_total, average);

    char esc_name[MAX_CAT_LEN * 2];
    for (size_t c = 0; c < cat_count; c++) {
        double pct = (grand_total > 0.0) ? (cats[c].total / grand_total) * 100.0 : 0.0;
        json_escape(cats[c].name, esc_name, sizeof(esc_name));

        offset += snprintf(json + offset, json_size - offset,
            "    {\"name\": \"%s\", \"total\": %.2f, \"count\": %lu, \"percentage\": %.2f}%s\n",
            esc_name, cats[c].total, (unsigned long)cats[c].count, pct,
            (c + 1 < cat_count) ? "," : "");
    }

    offset += snprintf(json + offset, json_size - offset, "  ]");

    if (highest) {
        char esc_desc[MAX_DESC_LEN * 2];
        json_escape(highest->description, esc_desc, sizeof(esc_desc));
        offset += snprintf(json + offset, json_size - offset,
            ",\n  \"highest\": {\"id\": %d, \"amount\": %.2f, \"category\": \"%s\", \"date\": \"%s\", \"description\": \"%s\"}",
            highest->id, highest->amount, highest->category, highest->date, esc_desc);
    } else {
        offset += snprintf(json + offset, json_size - offset, ",\n  \"highest\": null");
    }

    if (lowest) {
        char esc_desc[MAX_DESC_LEN * 2];
        json_escape(lowest->description, esc_desc, sizeof(esc_desc));
        offset += snprintf(json + offset, json_size - offset,
            ",\n  \"lowest\": {\"id\": %d, \"amount\": %.2f, \"category\": \"%s\", \"date\": \"%s\", \"description\": \"%s\"}",
            lowest->id, lowest->amount, lowest->category, lowest->date, esc_desc);
    } else {
        offset += snprintf(json + offset, json_size - offset, ",\n  \"lowest\": null");
    }

    offset += snprintf(json + offset, json_size - offset, "\n}\n");

    send_response(client, 200, "OK", "application/json", json, offset);
    free(json);
}

static void handle_add_expense_api(SOCKET client, const char *body) {
    char date[MAX_DATE_LEN] = "";
    char cat[MAX_CAT_LEN] = "";
    char desc[MAX_DESC_LEN] = "";
    double amt = 0.0;

    extract_json_string(body, "date", date, sizeof(date));
    extract_json_string(body, "category", cat, sizeof(cat));
    extract_json_string(body, "description", desc, sizeof(desc));
    extract_json_number(body, "amount", &amt);

    if (date[0] == '\0') {
        get_current_date(date, sizeof(date));
    }
    if (cat[0] == '\0') {
        strncpy(cat, "Other", sizeof(cat) - 1);
    }

    if (amt <= 0.0) {
        const char *err = "{\"error\": \"Amount must be greater than zero.\"}";
        send_response(client, 400, "Bad Request", "application/json", err, strlen(err));
        return;
    }

    int id = expense_list_add(&g_expense_list, date, cat, amt, desc);
    if (id > 0) {
        expense_list_save_csv(&g_expense_list, DEFAULT_FILE_NAME);
        char resp[256];
        int len = snprintf(resp, sizeof(resp), "{\"success\": true, \"id\": %d}", id);
        send_response(client, 201, "Created", "application/json", resp, len);
    } else {
        const char *err = "{\"error\": \"Failed to add expense.\"}";
        send_response(client, 500, "Internal Server Error", "application/json", err, strlen(err));
    }
}

static void handle_update_expense_api(SOCKET client, const char *body) {
    double id_val = 0.0;
    if (!extract_json_number(body, "id", &id_val) || id_val <= 0.0) {
        const char *err = "{\"error\": \"Valid expense ID is required.\"}";
        send_response(client, 400, "Bad Request", "application/json", err, strlen(err));
        return;
    }
    int id = (int)id_val;

    char date[MAX_DATE_LEN] = "";
    char cat[MAX_CAT_LEN] = "";
    char desc[MAX_DESC_LEN] = "";
    double amt = -1.0;

    extract_json_string(body, "date", date, sizeof(date));
    extract_json_string(body, "category", cat, sizeof(cat));
    extract_json_string(body, "description", desc, sizeof(desc));
    extract_json_number(body, "amount", &amt);

    if (expense_list_update(&g_expense_list, id,
                            date[0] ? date : NULL,
                            cat[0] ? cat : NULL,
                            amt,
                            desc[0] ? desc : NULL)) {
        expense_list_save_csv(&g_expense_list, DEFAULT_FILE_NAME);
        const char *resp = "{\"success\": true}";
        send_response(client, 200, "OK", "application/json", resp, strlen(resp));
    } else {
        const char *err = "{\"error\": \"Expense not found.\"}";
        send_response(client, 404, "Not Found", "application/json", err, strlen(err));
    }
}

static void handle_delete_expense_api(SOCKET client, const char *body, const char *query) {
    int id = 0;
    double id_val = 0.0;

    if (body && extract_json_number(body, "id", &id_val) && id_val > 0.0) {
        id = (int)id_val;
    } else if (query && strstr(query, "id=")) {
        const char *p = strstr(query, "id=") + 3;
        id = atoi(p);
    }

    if (id <= 0) {
        const char *err = "{\"error\": \"Invalid or missing ID.\"}";
        send_response(client, 400, "Bad Request", "application/json", err, strlen(err));
        return;
    }

    if (expense_list_delete(&g_expense_list, id)) {
        expense_list_save_csv(&g_expense_list, DEFAULT_FILE_NAME);
        const char *resp = "{\"success\": true}";
        send_response(client, 200, "OK", "application/json", resp, strlen(resp));
    } else {
        const char *err = "{\"error\": \"Expense not found.\"}";
        send_response(client, 404, "Not Found", "application/json", err, strlen(err));
    }
}

int start_http_server(int port, const char *web_root) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "Failed to initialize Winsock. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    expense_list_init(&g_expense_list);
    expense_list_load_csv(&g_expense_list, DEFAULT_FILE_NAME);

    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {
        fprintf(stderr, "Could not create socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind failed with error code: %d\n", WSAGetLastError());
        closesocket(server_sock);
        WSACleanup();
        return 1;
    }

    listen(server_sock, 10);

    printf("\n=================================================================\n");
    printf("   EXPENSE TRACKER WEB SERVER RUNNING (NATIVE C BACKEND)         \n");
    printf("=================================================================\n");
    printf("   URL: http://localhost:%d\n", port);
    printf("   Database: %s (%lu records loaded)\n", DEFAULT_FILE_NAME, (unsigned long)g_expense_list.count);
    printf("   Press Ctrl+C in this terminal to stop the server.\n");
    printf("=================================================================\n\n");

    char buffer[BUFFER_SIZE];

    while (1) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        SOCKET client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock == INVALID_SOCKET) {
            continue;
        }

        int bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            closesocket(client_sock);
            continue;
        }
        buffer[bytes_received] = '\0';

        // Check for Expect: 100-continue and acknowledge
        if (strstr(buffer, "100-continue") || strstr(buffer, "100-Continue")) {
            const char *cont = "HTTP/1.1 100 Continue\r\n\r\n";
            send(client_sock, cont, (int)strlen(cont), 0);
        }

        // Locate HTTP request body and ensure full content is received
        char *header_end = strstr(buffer, "\r\n\r\n");
        if (header_end) {
            int header_len = (int)(header_end + 4 - buffer);
            int body_received = bytes_received - header_len;

            int content_length = 0;
            char *cl = strstr(buffer, "Content-Length:");
            if (!cl) cl = strstr(buffer, "content-length:");
            if (cl) {
                content_length = atoi(cl + 15);
            }

            while (body_received < content_length && bytes_received < BUFFER_SIZE - 1) {
                int r = recv(client_sock, buffer + bytes_received, (BUFFER_SIZE - 1) - bytes_received, 0);
                if (r <= 0) break;
                bytes_received += r;
                body_received += r;
            }
            buffer[bytes_received] = '\0';
        }

        char method[16] = {0};
        char url[256] = {0};
        sscanf(buffer, "%15s %255s", method, url);

        // Separate query string if any
        char *query = strchr(url, '?');
        if (query) {
            *query = '\0';
            query++;
        }

        const char *body = header_end ? (header_end + 4) : "";

        if (strcmp(method, "OPTIONS") == 0) {
            send_response(client_sock, 204, "No Content", "text/plain", "", 0);
        } else if (strcmp(method, "GET") == 0) {
            if (strcmp(url, "/") == 0 || strcmp(url, "/index.html") == 0) {
                char path[512];
                snprintf(path, sizeof(path), "%s/index.html", web_root);
                serve_file(client_sock, path, "text/html; charset=utf-8");
            } else if (strcmp(url, "/style.css") == 0) {
                char path[512];
                snprintf(path, sizeof(path), "%s/style.css", web_root);
                serve_file(client_sock, path, "text/css; charset=utf-8");
            } else if (strcmp(url, "/app.js") == 0) {
                char path[512];
                snprintf(path, sizeof(path), "%s/app.js", web_root);
                serve_file(client_sock, path, "application/javascript; charset=utf-8");
            } else if (strcmp(url, "/api/expenses") == 0) {
                handle_get_expenses(client_sock);
            } else if (strcmp(url, "/api/stats") == 0) {
                handle_get_stats(client_sock);
            } else {
                char path[512];
                snprintf(path, sizeof(path), "%s%s", web_root, url);
                serve_file(client_sock, path, "application/octet-stream");
            }
        } else if (strcmp(method, "POST") == 0) {
            if (strcmp(url, "/api/expenses") == 0) {
                handle_add_expense_api(client_sock, body);
            } else if (strcmp(url, "/api/expenses/update") == 0) {
                handle_update_expense_api(client_sock, body);
            } else if (strcmp(url, "/api/expenses/delete") == 0) {
                handle_delete_expense_api(client_sock, body, query);
            } else {
                const char *err = "{\"error\": \"Endpoint not found\"}";
                send_response(client_sock, 404, "Not Found", "application/json", err, strlen(err));
            }
        } else if (strcmp(method, "DELETE") == 0) {
            if (strcmp(url, "/api/expenses") == 0) {
                handle_delete_expense_api(client_sock, body, query);
            } else {
                const char *err = "{\"error\": \"Endpoint not found\"}";
                send_response(client_sock, 404, "Not Found", "application/json", err, strlen(err));
            }
        } else {
            const char *err = "{\"error\": \"Method not supported\"}";
            send_response(client_sock, 405, "Method Not Allowed", "application/json", err, strlen(err));
        }

        closesocket(client_sock);
    }

    closesocket(server_sock);
    WSACleanup();
    expense_list_free(&g_expense_list);
    return 0;
}
