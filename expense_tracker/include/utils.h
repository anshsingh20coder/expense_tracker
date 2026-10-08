#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

/**
 * Reads a line of text safely from stdin, removing trailing newline characters
 * and preventing buffer overflows.
 */
void get_safe_string(char *buffer, size_t size);

/**
 * Safely reads an integer from stdin.
 * Returns 1 on success, 0 on invalid input.
 */
int get_safe_int(int *out_val);

/**
 * Safely reads a positive double/float value from stdin.
 * Returns 1 on success, 0 on invalid input.
 */
int get_safe_double(double *out_val);

/**
 * Validates whether the given string is in valid YYYY-MM-DD format
 * and corresponds to a real calendar date (handles leap years).
 * Returns 1 if valid, 0 otherwise.
 */
int is_valid_date(const char *date_str);

/**
 * Gets the current system date formatted as YYYY-MM-DD.
 */
void get_current_date(char *buffer, size_t size);

/**
 * Trims leading and trailing whitespace characters in place.
 */
void trim_whitespace(char *str);

/**
 * Pauses execution until the user presses Enter.
 */
void pause_prompt(void);

/**
 * Clears the console screen cross-platform.
 */
void clear_screen(void);

/**
 * Compares two dates in YYYY-MM-DD format.
 * Returns < 0 if d1 < d2, 0 if equal, > 0 if d1 > d2.
 */
int compare_dates(const char *d1, const char *d2);

#endif /* UTILS_H */
