#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

void get_safe_string(char *buffer, size_t size) {
    if (size == 0 || buffer == NULL) return;

    if (fgets(buffer, (int)size, stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
            // Trim newline characters
            while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
                buffer[len - 1] = '\0';
                len--;
            }
        } else if (len == size - 1) {
            // Input exceeded buffer size, flush the rest of the line
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF);
        }
    } else {
        buffer[0] = '\0';
        if (feof(stdin)) {
            // Reached End-Of-File (pipe closed or Ctrl+D/Ctrl+Z)
            exit(0);
        }
    }
}

void trim_whitespace(char *str) {
    if (str == NULL) return;

    // Trim leading space
    char *start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    // Trim trailing space
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
}

int get_safe_int(int *out_val) {
    char buf[64];
    get_safe_string(buf, sizeof(buf));
    trim_whitespace(buf);

    if (buf[0] == '\0') {
        return 0;
    }

    char *endptr;
    long val = strtol(buf, &endptr, 10);
    if (*endptr != '\0') {
        return 0; // Contains non-numeric characters
    }

    if (out_val) {
        *out_val = (int)val;
    }
    return 1;
}

int get_safe_double(double *out_val) {
    char buf[64];
    get_safe_string(buf, sizeof(buf));
    trim_whitespace(buf);

    if (buf[0] == '\0') {
        return 0;
    }

    char *endptr;
    double val = strtod(buf, &endptr);
    if (*endptr != '\0') {
        return 0;
    }

    if (val < 0.0) {
        return 0; // Disallow negative expense amounts
    }

    if (out_val) {
        *out_val = val;
    }
    return 1;
}

int is_valid_date(const char *date_str) {
    if (date_str == NULL || strlen(date_str) != 10) {
        return 0;
    }

    // Format must strictly match YYYY-MM-DD
    if (date_str[4] != '-' || date_str[7] != '-') {
        return 0;
    }

    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) continue;
        if (!isdigit((unsigned char)date_str[i])) {
            return 0;
        }
    }

    int year, month, day;
    if (sscanf(date_str, "%4d-%2d-%2d", &year, &month, &day) != 3) {
        return 0;
    }

    if (year < 1900 || year > 2100) return 0;
    if (month < 1 || month > 12) return 0;

    int days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    // Check for leap year
    int is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (month == 2 && is_leap) {
        days_in_month[1] = 29;
    }

    if (day < 1 || day > days_in_month[month - 1]) {
        return 0;
    }

    return 1;
}

void get_current_date(char *buffer, size_t size) {
    if (buffer == NULL || size < 11) return;

    time_t raw_time = time(NULL);
    struct tm *info = localtime(&raw_time);
    if (info != NULL) {
        strftime(buffer, size, "%Y-%m-%d", info);
    } else {
        snprintf(buffer, size, "2026-01-01");
    }
}

int compare_dates(const char *d1, const char *d2) {
    return strcmp(d1, d2);
}

void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pause_prompt(void) {
    printf("\nPress [Enter] to continue...");
    char dummy[16];
    get_safe_string(dummy, sizeof(dummy));
}
