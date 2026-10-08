#include "expense.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const char *PRESET_CATEGORIES[] = {
    "Food & Dining",
    "Transportation",
    "Housing & Utilities",
    "Entertainment",
    "Healthcare",
    "Shopping",
    "Education",
    "Personal Care",
    "Travel",
    "Other"
};

const int PRESET_CATEGORIES_COUNT = sizeof(PRESET_CATEGORIES) / sizeof(PRESET_CATEGORIES[0]);

void expense_list_init(ExpenseList *list) {
    if (!list) return;
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    list->next_id = 1;
}

void expense_list_free(ExpenseList *list) {
    if (!list) return;
    if (list->items) {
        free(list->items);
        list->items = NULL;
    }
    list->count = 0;
    list->capacity = 0;
    list->next_id = 1;
}

static int ensure_capacity(ExpenseList *list, size_t min_capacity) {
    if (list->capacity >= min_capacity) return 1;

    size_t new_capacity = (list->capacity == 0) ? 16 : list->capacity * 2;
    if (new_capacity < min_capacity) {
        new_capacity = min_capacity;
    }

    Expense *new_items = (Expense *)realloc(list->items, new_capacity * sizeof(Expense));
    if (!new_items) {
        fprintf(stderr, "Error: Memory allocation failed!\n");
        return 0;
    }

    list->items = new_items;
    list->capacity = new_capacity;
    return 1;
}

int expense_list_add(ExpenseList *list, const char *date, const char *category, double amount, const char *description) {
    if (!list || !date || !category) return -1;

    if (!ensure_capacity(list, list->count + 1)) {
        return -1;
    }

    Expense *e = &list->items[list->count];
    e->id = list->next_id++;
    strncpy(e->date, date, MAX_DATE_LEN - 1);
    e->date[MAX_DATE_LEN - 1] = '\0';

    strncpy(e->category, category, MAX_CAT_LEN - 1);
    e->category[MAX_CAT_LEN - 1] = '\0';

    e->amount = amount;

    if (description) {
        strncpy(e->description, description, MAX_DESC_LEN - 1);
        e->description[MAX_DESC_LEN - 1] = '\0';
    } else {
        e->description[0] = '\0';
    }

    list->count++;
    return e->id;
}

Expense *expense_list_find_by_id(ExpenseList *list, int id) {
    if (!list) return NULL;
    for (size_t i = 0; i < list->count; i++) {
        if (list->items[i].id == id) {
            return &list->items[i];
        }
    }
    return NULL;
}

int expense_list_update(ExpenseList *list, int id, const char *date, const char *category, double amount, const char *description) {
    Expense *e = expense_list_find_by_id(list, id);
    if (!e) return 0;

    if (date && date[0] != '\0') {
        strncpy(e->date, date, MAX_DATE_LEN - 1);
        e->date[MAX_DATE_LEN - 1] = '\0';
    }

    if (category && category[0] != '\0') {
        strncpy(e->category, category, MAX_CAT_LEN - 1);
        e->category[MAX_CAT_LEN - 1] = '\0';
    }

    if (amount >= 0.0) {
        e->amount = amount;
    }

    if (description && description[0] != '\0') {
        strncpy(e->description, description, MAX_DESC_LEN - 1);
        e->description[MAX_DESC_LEN - 1] = '\0';
    }

    return 1;
}

int expense_list_delete(ExpenseList *list, int id) {
    if (!list) return 0;

    for (size_t i = 0; i < list->count; i++) {
        if (list->items[i].id == id) {
            // Shift remaining items
            for (size_t j = i; j < list->count - 1; j++) {
                list->items[j] = list->items[j + 1];
            }
            list->count--;
            return 1;
        }
    }
    return 0;
}

static void escape_csv_field(const char *src, char *dest, size_t dest_size) {
    int needs_quotes = 0;
    if (strchr(src, ',') || strchr(src, '"') || strchr(src, '\n') || strchr(src, '\r')) {
        needs_quotes = 1;
    }

    if (!needs_quotes) {
        strncpy(dest, src, dest_size - 1);
        dest[dest_size - 1] = '\0';
        return;
    }

    size_t pos = 0;
    if (pos < dest_size - 1) dest[pos++] = '"';

    for (size_t i = 0; src[i] != '\0' && pos < dest_size - 2; i++) {
        if (src[i] == '"') {
            dest[pos++] = '"';
        }
        if (pos < dest_size - 2) {
            dest[pos++] = src[i];
        }
    }

    if (pos < dest_size - 1) dest[pos++] = '"';
    dest[pos] = '\0';
}

int expense_list_save_csv(const ExpenseList *list, const char *filepath) {
    if (!list || !filepath) return 0;

    FILE *f = fopen(filepath, "w");
    if (!f) {
        fprintf(stderr, "Error: Could not open file '%s' for writing.\n", filepath);
        return 0;
    }

    // Header
    fprintf(f, "ID,Date,Category,Amount,Description\n");

    char esc_cat[MAX_CAT_LEN * 2];
    char esc_desc[MAX_DESC_LEN * 2];

    for (size_t i = 0; i < list->count; i++) {
        const Expense *e = &list->items[i];
        escape_csv_field(e->category, esc_cat, sizeof(esc_cat));
        escape_csv_field(e->description, esc_desc, sizeof(esc_desc));
        fprintf(f, "%d,%s,%s,%.2f,%s\n", e->id, e->date, esc_cat, e->amount, esc_desc);
    }

    fclose(f);
    return 1;
}

static int parse_csv_line(char *line, int *out_id, char *out_date, char *out_cat, double *out_amt, char *out_desc) {
    trim_whitespace(line);
    if (line[0] == '\0') return 0;

    char *p = line;
    char fields[5][MAX_DESC_LEN];
    int field_idx = 0;

    while (*p != '\0' && field_idx < 5) {
        char buf[MAX_DESC_LEN];
        size_t b_idx = 0;

        if (*p == '"') {
            p++; // Skip opening quote
            while (*p != '\0') {
                if (*p == '"') {
                    if (*(p + 1) == '"') {
                        // Escaped quote
                        if (b_idx < sizeof(buf) - 1) buf[b_idx++] = '"';
                        p += 2;
                    } else {
                        // Closing quote
                        p++;
                        break;
                    }
                } else {
                    if (b_idx < sizeof(buf) - 1) buf[b_idx++] = *p;
                    p++;
                }
            }
            if (*p == ',') p++;
        } else {
            // Unquoted field
            while (*p != '\0' && *p != ',') {
                if (b_idx < sizeof(buf) - 1) buf[b_idx++] = *p;
                p++;
            }
            if (*p == ',') p++;
        }
        buf[b_idx] = '\0';
        strncpy(fields[field_idx], buf, MAX_DESC_LEN - 1);
        fields[field_idx][MAX_DESC_LEN - 1] = '\0';
        field_idx++;
    }

    if (field_idx < 4) return 0; // At least ID, Date, Category, Amount required

    *out_id = atoi(fields[0]);
    strncpy(out_date, fields[1], MAX_DATE_LEN - 1);
    out_date[MAX_DATE_LEN - 1] = '\0';

    strncpy(out_cat, fields[2], MAX_CAT_LEN - 1);
    out_cat[MAX_CAT_LEN - 1] = '\0';

    *out_amt = atof(fields[3]);

    if (field_idx >= 5) {
        strncpy(out_desc, fields[4], MAX_DESC_LEN - 1);
        out_desc[MAX_DESC_LEN - 1] = '\0';
    } else {
        out_desc[0] = '\0';
    }

    return 1;
}

int expense_list_load_csv(ExpenseList *list, const char *filepath) {
    if (!list || !filepath) return -1;

    FILE *f = fopen(filepath, "r");
    if (!f) {
        // File doesn't exist yet, start fresh
        return 0;
    }

    char line[512];
    int line_num = 0;
    int loaded = 0;

    while (fgets(line, sizeof(line), f) != NULL) {
        line_num++;
        if (line_num == 1) {
            // Check for header
            if (strncmp(line, "ID,", 3) == 0 || strncmp(line, "id,", 3) == 0) {
                continue;
            }
        }

        int id;
        char date[MAX_DATE_LEN];
        char cat[MAX_CAT_LEN];
        double amt;
        char desc[MAX_DESC_LEN];

        if (parse_csv_line(line, &id, date, cat, &amt, desc)) {
            if (ensure_capacity(list, list->count + 1)) {
                Expense *e = &list->items[list->count++];
                e->id = id;
                strncpy(e->date, date, MAX_DATE_LEN - 1);
                e->date[MAX_DATE_LEN - 1] = '\0';
                strncpy(e->category, cat, MAX_CAT_LEN - 1);
                e->category[MAX_CAT_LEN - 1] = '\0';
                e->amount = amt;
                strncpy(e->description, desc, MAX_DESC_LEN - 1);
                e->description[MAX_DESC_LEN - 1] = '\0';

                if (id >= list->next_id) {
                    list->next_id = id + 1;
                }
                loaded++;
            }
        }
    }

    fclose(f);
    return loaded;
}

void display_expenses_table(const Expense *items, size_t count, const char *title) {
    if (title && title[0] != '\0') {
        printf("\n=== %s ===\n", title);
    }

    if (count == 0) {
        printf("\n[ No expenses found matching your criteria. ]\n");
        return;
    }

    printf("+------+------------+---------------------+------------+--------------------------------+\n");
    printf("|  ID  |    Date    | Category            |   Amount   | Description                    |\n");
    printf("+------+------------+---------------------+------------+--------------------------------+\n");

    double total = 0.0;
    for (size_t i = 0; i < count; i++) {
        const Expense *e = &items[i];
        total += e->amount;

        char truncated_desc[32];
        if (strlen(e->description) > 30) {
            strncpy(truncated_desc, e->description, 27);
            truncated_desc[27] = '.';
            truncated_desc[28] = '.';
            truncated_desc[29] = '.';
            truncated_desc[30] = '\0';
        } else {
            strncpy(truncated_desc, e->description, sizeof(truncated_desc) - 1);
            truncated_desc[sizeof(truncated_desc) - 1] = '\0';
        }

        printf("| %4d | %-10s | %-19s | %10.2f | %-30s |\n",
               e->id, e->date, e->category, e->amount, truncated_desc);
    }

    printf("+------+------------+---------------------+------------+--------------------------------+\n");
    printf("| Total: %-4lu item(s)                       | %10.2f |                                |\n",
           (unsigned long)count, total);
    printf("+------+------------+---------------------+------------+--------------------------------+\n");
}

void display_all_expenses(const ExpenseList *list) {
    if (!list) return;
    display_expenses_table(list->items, list->count, "ALL RECORDED EXPENSES");
}

static int case_insensitive_contains(const char *haystack, const char *needle) {
    if (!haystack || !needle) return 0;
    if (needle[0] == '\0') return 1;

    size_t hlen = strlen(haystack);
    size_t nlen = strlen(needle);
    if (nlen > hlen) return 0;

    for (size_t i = 0; i <= hlen - nlen; i++) {
        size_t j;
        for (j = 0; j < nlen; j++) {
            if (tolower((unsigned char)haystack[i + j]) != tolower((unsigned char)needle[j])) {
                break;
            }
        }
        if (j == nlen) return 1;
    }
    return 0;
}

void filter_by_category(const ExpenseList *list, const char *category) {
    if (!list || !category) return;

    Expense *matches = (Expense *)malloc(list->count * sizeof(Expense));
    if (!matches && list->count > 0) return;

    size_t match_count = 0;
    for (size_t i = 0; i < list->count; i++) {
        if (case_insensitive_contains(list->items[i].category, category)) {
            matches[match_count++] = list->items[i];
        }
    }

    char title[128];
    snprintf(title, sizeof(title), "EXPENSES IN CATEGORY: '%s'", category);
    display_expenses_table(matches, match_count, title);

    free(matches);
}

void filter_by_date_range(const ExpenseList *list, const char *start_date, const char *end_date) {
    if (!list || !start_date || !end_date) return;

    Expense *matches = (Expense *)malloc(list->count * sizeof(Expense));
    if (!matches && list->count > 0) return;

    size_t match_count = 0;
    for (size_t i = 0; i < list->count; i++) {
        if (compare_dates(list->items[i].date, start_date) >= 0 &&
            compare_dates(list->items[i].date, end_date) <= 0) {
            matches[match_count++] = list->items[i];
        }
    }

    char title[128];
    snprintf(title, sizeof(title), "EXPENSES BETWEEN %s AND %s", start_date, end_date);
    display_expenses_table(matches, match_count, title);

    free(matches);
}

void filter_by_month(const ExpenseList *list, int year, int month) {
    if (!list) return;

    char prefix[16];
    snprintf(prefix, sizeof(prefix), "%04d-%02d", year, month);

    Expense *matches = (Expense *)malloc(list->count * sizeof(Expense));
    if (!matches && list->count > 0) return;

    size_t match_count = 0;
    for (size_t i = 0; i < list->count; i++) {
        if (strncmp(list->items[i].date, prefix, 7) == 0) {
            matches[match_count++] = list->items[i];
        }
    }

    char title[128];
    snprintf(title, sizeof(title), "EXPENSES FOR %04d-%02d", year, month);
    display_expenses_table(matches, match_count, title);

    free(matches);
}

void search_expenses(const ExpenseList *list, const char *keyword) {
    if (!list || !keyword) return;

    Expense *matches = (Expense *)malloc(list->count * sizeof(Expense));
    if (!matches && list->count > 0) return;

    size_t match_count = 0;
    for (size_t i = 0; i < list->count; i++) {
        if (case_insensitive_contains(list->items[i].description, keyword) ||
            case_insensitive_contains(list->items[i].category, keyword)) {
            matches[match_count++] = list->items[i];
        }
    }

    char title[128];
    snprintf(title, sizeof(title), "SEARCH RESULTS FOR: '%s'", keyword);
    display_expenses_table(matches, match_count, title);

    free(matches);
}

typedef struct {
    char name[MAX_CAT_LEN];
    double total;
    size_t count;
} CategoryStat;

static void print_report_stream(FILE *out, const ExpenseList *list) {
    if (list->count == 0) {
        fprintf(out, "\n[ No expenses recorded yet. ]\n");
        return;
    }

    double grand_total = 0.0;
    double max_amt = list->items[0].amount;
    double min_amt = list->items[0].amount;
    const Expense *highest_expense = &list->items[0];
    const Expense *lowest_expense = &list->items[0];

    // Collect category totals
    CategoryStat cats[64];
    size_t cat_count = 0;

    for (size_t i = 0; i < list->count; i++) {
        const Expense *e = &list->items[i];
        grand_total += e->amount;

        if (e->amount > max_amt) {
            max_amt = e->amount;
            highest_expense = e;
        }
        if (e->amount < min_amt) {
            min_amt = e->amount;
            lowest_expense = e;
        }

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

    double average_expense = grand_total / (double)list->count;

    fprintf(out, "====================================================================\n");
    fprintf(out, "                     EXPENSE SUMMARY & ANALYTICS                    \n");
    fprintf(out, "====================================================================\n");
    fprintf(out, "  Total Records     : %lu\n", (unsigned long)list->count);
    fprintf(out, "  Total Expenditure : Rs. %.2f\n", grand_total);
    fprintf(out, "  Average Expense   : Rs. %.2f\n", average_expense);
    fprintf(out, "  Highest Expense   : Rs. %.2f (#%d - %s [%s]: \"%s\")\n",
            highest_expense->amount, highest_expense->id, highest_expense->date,
            highest_expense->category, highest_expense->description);
    fprintf(out, "  Lowest Expense    : Rs. %.2f (#%d - %s [%s]: \"%s\")\n",
            lowest_expense->amount, lowest_expense->id, lowest_expense->date,
            lowest_expense->category, lowest_expense->description);
    fprintf(out, "--------------------------------------------------------------------\n");
    fprintf(out, "CATEGORY BREAKDOWN:\n");
    fprintf(out, "--------------------------------------------------------------------\n");
    fprintf(out, "%-22s | %-5s | %-10s | %-6s | %s\n", "Category", "Count", "Total (Rs)", "%", "Visual Distribution");
    fprintf(out, "-----------------------+-------+------------+--------+----------------------\n");

    for (size_t c = 0; c < cat_count; c++) {
        double pct = (grand_total > 0.0) ? (cats[c].total / grand_total) * 100.0 : 0.0;
        int bar_width = (int)(pct / 5.0); // 20 chars max for 100%
        if (bar_width > 20) bar_width = 20;

        char bar[32];
        for (int b = 0; b < bar_width; b++) bar[b] = '#';
        for (int b = bar_width; b < 20; b++) bar[b] = ' ';
        bar[20] = '\0';

        fprintf(out, "%-22s | %-5lu | %10.2f | %5.1f%% | [%s]\n",
                cats[c].name, (unsigned long)cats[c].count, cats[c].total, pct, bar);
    }
    fprintf(out, "====================================================================\n");
}

void generate_statistics(const ExpenseList *list, const char *export_filepath) {
    if (!list) return;

    print_report_stream(stdout, list);

    if (export_filepath && export_filepath[0] != '\0') {
        FILE *f = fopen(export_filepath, "w");
        if (f) {
            print_report_stream(f, list);
            fclose(f);
            printf("\n>> Summary report successfully saved to '%s'!\n", export_filepath);
        } else {
            fprintf(stderr, "\nWarning: Failed to export report to '%s'.\n", export_filepath);
        }
    }
}

static int cmp_date_asc(const void *a, const void *b) {
    return strcmp(((const Expense *)a)->date, ((const Expense *)b)->date);
}

static int cmp_date_desc(const void *a, const void *b) {
    return strcmp(((const Expense *)b)->date, ((const Expense *)a)->date);
}

static int cmp_amt_asc(const void *a, const void *b) {
    double diff = ((const Expense *)a)->amount - ((const Expense *)b)->amount;
    return (diff > 0) - (diff < 0);
}

static int cmp_amt_desc(const void *a, const void *b) {
    double diff = ((const Expense *)b)->amount - ((const Expense *)a)->amount;
    return (diff > 0) - (diff < 0);
}

void sort_by_date(ExpenseList *list, int asc) {
    if (!list || list->count <= 1) return;
    qsort(list->items, list->count, sizeof(Expense), asc ? cmp_date_asc : cmp_date_desc);
}

void sort_by_amount(ExpenseList *list, int asc) {
    if (!list || list->count <= 1) return;
    qsort(list->items, list->count, sizeof(Expense), asc ? cmp_amt_asc : cmp_amt_desc);
}
