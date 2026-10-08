#ifndef EXPENSE_H
#define EXPENSE_H

#include <stddef.h>

#define MAX_DATE_LEN 12       /* "YYYY-MM-DD\0" */
#define MAX_CAT_LEN 32
#define MAX_DESC_LEN 128
#define DEFAULT_FILE_NAME "expenses.csv"

typedef struct {
    int id;
    char date[MAX_DATE_LEN];
    char category[MAX_CAT_LEN];
    double amount;
    char description[MAX_DESC_LEN];
} Expense;

typedef struct {
    Expense *items;
    size_t count;
    size_t capacity;
    int next_id;
} ExpenseList;

/* Predefined standard categories */
extern const char *PRESET_CATEGORIES[];
extern const int PRESET_CATEGORIES_COUNT;

/**
 * Initializes an empty expense list.
 */
void expense_list_init(ExpenseList *list);

/**
 * Frees allocated memory for the expense list.
 */
void expense_list_free(ExpenseList *list);

/**
 * Loads expenses from a CSV file. If the file doesn't exist, it creates a clean list.
 * Returns number of expenses loaded, or -1 on error.
 */
int expense_list_load_csv(ExpenseList *list, const char *filepath);

/**
 * Saves all current expenses to a CSV file.
 * Returns 1 on success, 0 on failure.
 */
int expense_list_save_csv(const ExpenseList *list, const char *filepath);

/**
 * Adds an expense to the list and auto-increments next_id.
 * Returns the assigned ID on success, or -1 on allocation failure.
 */
int expense_list_add(ExpenseList *list, const char *date, const char *category, double amount, const char *description);

/**
 * Finds an expense by its unique ID.
 * Returns pointer to Expense if found, NULL otherwise.
 */
Expense *expense_list_find_by_id(ExpenseList *list, int id);

/**
 * Updates an existing expense by ID.
 * Passing NULL or negative values preserves the existing field.
 * Returns 1 on success, 0 if not found.
 */
int expense_list_update(ExpenseList *list, int id, const char *date, const char *category, double amount, const char *description);

/**
 * Deletes an expense by its unique ID.
 * Returns 1 if deleted, 0 if not found.
 */
int expense_list_delete(ExpenseList *list, int id);

/**
 * Displays an array of expenses in a clean, formatted ASCII table.
 */
void display_expenses_table(const Expense *items, size_t count, const char *title);

/**
 * Displays all recorded expenses.
 */
void display_all_expenses(const ExpenseList *list);

/**
 * Filters and displays expenses matching a specific category.
 */
void filter_by_category(const ExpenseList *list, const char *category);

/**
 * Filters and displays expenses between start_date and end_date (inclusive).
 */
void filter_by_date_range(const ExpenseList *list, const char *start_date, const char *end_date);

/**
 * Filters and displays expenses for a given year and month.
 */
void filter_by_month(const ExpenseList *list, int year, int month);

/**
 * Searches and displays expenses whose descriptions or categories contain the keyword.
 */
void search_expenses(const ExpenseList *list, const char *keyword);

/**
 * Generates and prints a complete statistical report (total expenditure,
 * category breakdown with percentage bars, top expense, lowest expense, average).
 * Optionally writes the report to a text file if export_filepath is non-NULL.
 */
void generate_statistics(const ExpenseList *list, const char *export_filepath);

/**
 * Sorts expenses in-place by date (ascending if asc!=0, descending if asc==0).
 */
void sort_by_date(ExpenseList *list, int asc);

/**
 * Sorts expenses in-place by amount (ascending if asc!=0, descending if asc==0).
 */
void sort_by_amount(ExpenseList *list, int asc);

#endif /* EXPENSE_H */
