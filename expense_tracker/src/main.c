#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "expense.h"
#include "utils.h"

static void prompt_category(char *out_category, size_t size) {
    printf("\nSelect Category:\n");
    for (int i = 0; i < PRESET_CATEGORIES_COUNT; i++) {
        printf("  [%2d] %-22s", i + 1, PRESET_CATEGORIES[i]);
        if ((i + 1) % 2 == 0) printf("\n");
    }
    if (PRESET_CATEGORIES_COUNT % 2 != 0) printf("\n");
    printf("  [ 0] Custom Category\n");

    while (1) {
        printf("Enter choice (0-%d): ", PRESET_CATEGORIES_COUNT);
        int choice;
        if (get_safe_int(&choice)) {
            if (choice >= 1 && choice <= PRESET_CATEGORIES_COUNT) {
                strncpy(out_category, PRESET_CATEGORIES[choice - 1], size - 1);
                out_category[size - 1] = '\0';
                return;
            } else if (choice == 0) {
                printf("Enter custom category name: ");
                get_safe_string(out_category, size);
                trim_whitespace(out_category);
                if (out_category[0] != '\0') return;
                printf("Category cannot be empty. Please try again.\n");
            } else {
                printf("Invalid selection! Please choose between 0 and %d.\n", PRESET_CATEGORIES_COUNT);
            }
        } else {
            printf("Invalid input! Please enter a number.\n");
        }
    }
}

static void prompt_date(char *out_date, size_t size) {
    char today[MAX_DATE_LEN];
    get_current_date(today, sizeof(today));

    while (1) {
        printf("Enter date (YYYY-MM-DD) [Leave blank or type 't' for today: %s]: ", today);
        char input[32];
        get_safe_string(input, sizeof(input));
        trim_whitespace(input);

        if (input[0] == '\0' || strcmp(input, "t") == 0 || strcmp(input, "T") == 0) {
            strncpy(out_date, today, size - 1);
            out_date[size - 1] = '\0';
            return;
        }

        if (is_valid_date(input)) {
            strncpy(out_date, input, size - 1);
            out_date[size - 1] = '\0';
            return;
        } else {
            printf("Invalid date! Format must be YYYY-MM-DD (e.g., 2026-10-08). Try again.\n");
        }
    }
}

static void handle_add_expense(ExpenseList *list) {
    printf("\n--------------------------------------------------\n");
    printf("               ADD NEW EXPENSE\n");
    printf("--------------------------------------------------\n");

    char date[MAX_DATE_LEN];
    prompt_date(date, sizeof(date));

    char category[MAX_CAT_LEN];
    prompt_category(category, sizeof(category));

    double amount = 0.0;
    while (1) {
        printf("Enter amount (Rs.): ");
        if (get_safe_double(&amount) && amount > 0.0) {
            break;
        }
        printf("Invalid amount! Please enter a positive number greater than 0.\n");
    }

    char desc[MAX_DESC_LEN];
    printf("Enter description / note: ");
    get_safe_string(desc, sizeof(desc));
    trim_whitespace(desc);

    int id = expense_list_add(list, date, category, amount, desc);
    if (id > 0) {
        expense_list_save_csv(list, DEFAULT_FILE_NAME);
        printf("\n>> Success: Expense #%d added and saved successfully!\n", id);
    } else {
        printf("\n>> Error: Failed to add expense.\n");
    }
    pause_prompt();
}

static void handle_filter_submenu(const ExpenseList *list) {
    while (1) {
        printf("\n==================================================\n");
        printf("              FILTER & SEARCH EXPENSES            \n");
        printf("==================================================\n");
        printf("  [1] Filter by Category\n");
        printf("  [2] Filter by Date Range\n");
        printf("  [3] Filter by Specific Month (YYYY-MM)\n");
        printf("  [4] Search by Keyword in Description\n");
        printf("  [0] Return to Main Menu\n");
        printf("==================================================\n");
        printf("Choose an option: ");

        int choice;
        if (!get_safe_int(&choice)) {
            printf("Invalid selection! Try again.\n");
            continue;
        }

        if (choice == 0) break;

        switch (choice) {
            case 1: {
                char cat[MAX_CAT_LEN];
                printf("Enter category name or keyword: ");
                get_safe_string(cat, sizeof(cat));
                trim_whitespace(cat);
                if (cat[0] != '\0') {
                    filter_by_category(list, cat);
                } else {
                    printf("Category query cannot be empty.\n");
                }
                pause_prompt();
                break;
            }
            case 2: {
                char start_d[MAX_DATE_LEN], end_d[MAX_DATE_LEN];
                printf("Enter Start Date:\n");
                prompt_date(start_d, sizeof(start_d));
                printf("Enter End Date:\n");
                prompt_date(end_d, sizeof(end_d));

                if (compare_dates(start_d, end_d) > 0) {
                    printf("Error: Start date cannot be after end date.\n");
                } else {
                    filter_by_date_range(list, start_d, end_d);
                }
                pause_prompt();
                break;
            }
            case 3: {
                printf("Enter Year (e.g. 2026): ");
                int year;
                if (!get_safe_int(&year) || year < 1900 || year > 2100) {
                    printf("Invalid year entered.\n");
                    pause_prompt();
                    break;
                }
                printf("Enter Month (1-12): ");
                int month;
                if (!get_safe_int(&month) || month < 1 || month > 12) {
                    printf("Invalid month entered.\n");
                    pause_prompt();
                    break;
                }
                filter_by_month(list, year, month);
                pause_prompt();
                break;
            }
            case 4: {
                char kw[MAX_DESC_LEN];
                printf("Enter keyword to search: ");
                get_safe_string(kw, sizeof(kw));
                trim_whitespace(kw);
                if (kw[0] != '\0') {
                    search_expenses(list, kw);
                } else {
                    printf("Search keyword cannot be empty.\n");
                }
                pause_prompt();
                break;
            }
            default:
                printf("Invalid option. Please choose 0-4.\n");
                break;
        }
    }
}

static void handle_edit_expense(ExpenseList *list) {
    printf("\n--------------------------------------------------\n");
    printf("                  EDIT EXPENSE\n");
    printf("--------------------------------------------------\n");

    printf("Enter ID of the expense to edit: ");
    int id;
    if (!get_safe_int(&id)) {
        printf("Invalid ID entered.\n");
        pause_prompt();
        return;
    }

    Expense *e = expense_list_find_by_id(list, id);
    if (!e) {
        printf("Expense with ID #%d not found!\n", id);
        pause_prompt();
        return;
    }

    printf("\nCurrent Details:\n");
    printf("  ID          : %d\n", e->id);
    printf("  Date        : %s\n", e->date);
    printf("  Category    : %s\n", e->category);
    printf("  Amount      : Rs. %.2f\n", e->amount);
    printf("  Description : %s\n", e->description);
    printf("--------------------------------------------------\n");
    printf("Leave blank to keep current value.\n");

    char new_date[MAX_DATE_LEN] = "";
    printf("New Date (YYYY-MM-DD) [Blank = keep '%s']: ", e->date);
    char buf[64];
    get_safe_string(buf, sizeof(buf));
    trim_whitespace(buf);
    if (buf[0] != '\0') {
        if (is_valid_date(buf)) {
            strncpy(new_date, buf, sizeof(new_date) - 1);
        } else {
            printf("Invalid date! Keeping current date '%s'.\n", e->date);
        }
    }

    char new_cat[MAX_CAT_LEN] = "";
    printf("Change Category? (y/N): ");
    get_safe_string(buf, sizeof(buf));
    if (buf[0] == 'y' || buf[0] == 'Y') {
        prompt_category(new_cat, sizeof(new_cat));
    }

    double new_amt = -1.0;
    printf("New Amount (Rs.) [Blank = keep Rs. %.2f]: ", e->amount);
    get_safe_string(buf, sizeof(buf));
    trim_whitespace(buf);
    if (buf[0] != '\0') {
        char *endptr;
        double val = strtod(buf, &endptr);
        if (*endptr == '\0' && val > 0.0) {
            new_amt = val;
        } else {
            printf("Invalid amount! Keeping current amount Rs. %.2f.\n", e->amount);
        }
    }

    char new_desc[MAX_DESC_LEN] = "";
    printf("New Description [Blank = keep '%s']: ", e->description);
    get_safe_string(buf, sizeof(buf));
    trim_whitespace(buf);
    if (buf[0] != '\0') {
        strncpy(new_desc, buf, sizeof(new_desc) - 1);
    }

    expense_list_update(list, id,
                        new_date[0] ? new_date : NULL,
                        new_cat[0] ? new_cat : NULL,
                        new_amt,
                        new_desc[0] ? new_desc : NULL);

    expense_list_save_csv(list, DEFAULT_FILE_NAME);
    printf("\n>> Success: Expense #%d updated!\n", id);
    pause_prompt();
}

static void handle_delete_expense(ExpenseList *list) {
    printf("\n--------------------------------------------------\n");
    printf("                 DELETE EXPENSE\n");
    printf("--------------------------------------------------\n");

    printf("Enter ID of the expense to delete: ");
    int id;
    if (!get_safe_int(&id)) {
        printf("Invalid ID entered.\n");
        pause_prompt();
        return;
    }

    Expense *e = expense_list_find_by_id(list, id);
    if (!e) {
        printf("Expense with ID #%d not found!\n", id);
        pause_prompt();
        return;
    }

    printf("Are you sure you want to delete Expense #%d (%s - %s - Rs. %.2f)? (y/N): ",
           e->id, e->date, e->category, e->amount);
    char confirm[16];
    get_safe_string(confirm, sizeof(confirm));
    trim_whitespace(confirm);

    if (confirm[0] == 'y' || confirm[0] == 'Y') {
        if (expense_list_delete(list, id)) {
            expense_list_save_csv(list, DEFAULT_FILE_NAME);
            printf("\n>> Success: Expense #%d deleted.\n", id);
        } else {
            printf("\n>> Failed to delete expense.\n");
        }
    } else {
        printf("\nDeletion cancelled.\n");
    }
    pause_prompt();
}

static void handle_sort_submenu(ExpenseList *list) {
    printf("\n--------------------------------------------------\n");
    printf("                 SORT EXPENSES\n");
    printf("--------------------------------------------------\n");
    printf("  [1] Sort by Date (Newest first)\n");
    printf("  [2] Sort by Date (Oldest first)\n");
    printf("  [3] Sort by Amount (Highest first)\n");
    printf("  [4] Sort by Amount (Lowest first)\n");
    printf("Choose an option: ");

    int choice;
    if (!get_safe_int(&choice)) {
        printf("Invalid option.\n");
        pause_prompt();
        return;
    }

    switch (choice) {
        case 1: sort_by_date(list, 0); break;
        case 2: sort_by_date(list, 1); break;
        case 3: sort_by_amount(list, 0); break;
        case 4: sort_by_amount(list, 1); break;
        default:
            printf("Invalid choice.\n");
            pause_prompt();
            return;
    }

    expense_list_save_csv(list, DEFAULT_FILE_NAME);
    display_all_expenses(list);
    printf("\n>> Expenses sorted and list updated.\n");
    pause_prompt();
}

static void handle_export_report(const ExpenseList *list) {
    printf("\n--------------------------------------------------\n");
    printf("             EXPORT SUMMARY REPORT\n");
    printf("--------------------------------------------------\n");
    printf("Enter filename to export [Default: 'expense_report.txt']: ");
    char filename[128];
    get_safe_string(filename, sizeof(filename));
    trim_whitespace(filename);
    if (filename[0] == '\0') {
        strncpy(filename, "expense_report.txt", sizeof(filename) - 1);
    }

    generate_statistics(list, filename);
    pause_prompt();
}

int main(void) {
    ExpenseList list;
    expense_list_init(&list);

    int loaded = expense_list_load_csv(&list, DEFAULT_FILE_NAME);
    if (loaded > 0) {
        printf("[ Loaded %d expense record(s) from '%s' ]\n", loaded, DEFAULT_FILE_NAME);
    } else {
        printf("[ Starting with empty expense tracker database ]\n");
    }

    int running = 1;
    while (running) {
        printf("\n======================================================\n");
        printf("           PERSONAL EXPENSE TRACKER SYSTEM            \n");
        printf("======================================================\n");
        printf("  [1] Add New Expense\n");
        printf("  [2] View All Expenses\n");
        printf("  [3] Filter / Search Expenses\n");
        printf("  [4] View Expense Statistics & Analytics\n");
        printf("  [5] Edit an Expense\n");
        printf("  [6] Delete an Expense\n");
        printf("  [7] Sort Expenses\n");
        printf("  [8] Export Summary Report to File\n");
        printf("  [9] Exit Application\n");
        printf("======================================================\n");
        printf("Enter your choice (1-9): ");

        int choice;
        if (!get_safe_int(&choice)) {
            printf("Invalid input! Please enter a number between 1 and 9.\n");
            pause_prompt();
            continue;
        }

        switch (choice) {
            case 1:
                handle_add_expense(&list);
                break;
            case 2:
                display_all_expenses(&list);
                pause_prompt();
                break;
            case 3:
                handle_filter_submenu(&list);
                break;
            case 4:
                generate_statistics(&list, NULL);
                pause_prompt();
                break;
            case 5:
                handle_edit_expense(&list);
                break;
            case 6:
                handle_delete_expense(&list);
                break;
            case 7:
                handle_sort_submenu(&list);
                break;
            case 8:
                handle_export_report(&list);
                break;
            case 9:
                expense_list_save_csv(&list, DEFAULT_FILE_NAME);
                printf("\nSaving data to '%s'...\n", DEFAULT_FILE_NAME);
                printf("Thank you for using Expense Tracker System. Goodbye!\n\n");
                running = 0;
                break;
            default:
                printf("Invalid selection! Please enter a number between 1 and 9.\n");
                pause_prompt();
                break;
        }
    }

    expense_list_free(&list);
    return 0;
}
