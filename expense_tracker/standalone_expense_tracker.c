

#define MAX_DATE_LEN 12       /* Format: "YYYY-MM-DD\0" */
#define MAX_CAT_LEN  32
#define MAX_DESC_LEN 128
#define CSV_FILE     "expenses.csv"

/* --- Data Structures --- */

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

/* Predefined categories */
static const char *PRESET_CATEGORIES[] = {
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
static const int PRESET_CATEGORIES_COUNT = sizeof(PRESET_CATEGORIES) / sizeof(PRESET_CATEGORIES[0]);

/* --- Utility Functions --- */

void get_safe_string(char *buffer, size_t size) {
    if (size == 0 || buffer == NULL) return;

    if (fgets(buffer, (int)size, stdin) != NULL) {
        size_t len = strlen(buffer);
        while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
            buffer[len - 1] = '\0';
            len--;
        }
    } else {
        buffer[0] = '\0';
        if (feof(stdin)) {
            exit(0);
        }
    }
}

void trim_whitespace(char *str) {
    if (str == NULL) return;

    char *start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

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

    if (buf[0] == '\0') return 0;

    char *endptr;
    long val = strtol(buf, &endptr, 10);
    if (*endptr != '\0') return 0;

    if (out_val) *out_val = (int)val;
    return 1;
}

int get_safe_double(double *out_val) {
    char buf[64];
    get_safe_string(buf, sizeof(buf));
    trim_whitespace(buf);

    if (buf[0] == '\0') return 0;

    char *endptr;
    double val = strtod(buf, &endptr);
    if (*endptr != '\0' || val < 0.0) return 0;

    if (out_val) *out_val = val;
    return 1;
}

int is_valid_date(const char *date_str) {
    if (date_str == NULL || strlen(date_str) != 10) return 0;
    if (date_str[4] != '-' || date_str[7] != '-') return 0;

    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) continue;
        if (!isdigit((unsigned char)date_str[i])) return 0;
    }

    int year, month, day;
    if (sscanf(date_str, "%4d-%2d-%2d", &year, &month, &day) != 3) return 0;
    if (year < 1900 || year > 2100) return 0;
    if (month < 1 || month > 12) return 0;

    int days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (month == 2 && is_leap) days_in_month[1] = 29;

    if (day < 1 || day > days_in_month[month - 1]) return 0;
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

void pause_prompt(void) {
    printf("\nPress [Enter] to continue...");
    char dummy[16];
    get_safe_string(dummy, sizeof(dummy));
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

/* --- Expense List & CSV Operations --- */

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
    size_t new_cap = (list->capacity == 0) ? 16 : list->capacity * 2;
    if (new_cap < min_capacity) new_cap = min_capacity;

    Expense *new_items = (Expense *)realloc(list->items, new_cap * sizeof(Expense));
    if (!new_items) {
        fprintf(stderr, "Error: Memory reallocation failed.\n");
        return 0;
    }
    list->items = new_items;
    list->capacity = new_cap;
    return 1;
}

int expense_list_add(ExpenseList *list, const char *date, const char *category, double amount, const char *description) {
    if (!list || !date || !category) return -1;
    if (!ensure_capacity(list, list->count + 1)) return -1;

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
        if (list->items[i].id == id) return &list->items[i];
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
            for (size_t j = i; j < list->count - 1; j++) {
                list->items[j] = list->items[j + 1];
            }
            list->count--;
            return 1;
        }
    }
    return 0;
}

int expense_list_save_csv(const ExpenseList *list, const char *filepath) {
    if (!list || !filepath) return 0;
    FILE *f = fopen(filepath, "w");
    if (!f) return 0;

    fprintf(f, "ID,Date,Category,Amount,Description\n");
    for (size_t i = 0; i < list->count; i++) {
        const Expense *e = &list->items[i];
        fprintf(f, "%d,%s,%s,%.2f,%s\n", e->id, e->date, e->category, e->amount, e->description);
    }
    fclose(f);
    return 1;
}

int expense_list_load_csv(ExpenseList *list, const char *filepath) {
    if (!list || !filepath) return -1;
    FILE *f = fopen(filepath, "r");
    if (!f) return 0;

    char line[512];
    int line_num = 0;
    int loaded = 0;

    while (fgets(line, sizeof(line), f) != NULL) {
        line_num++;
        if (line_num == 1 && (strncmp(line, "ID,", 3) == 0 || strncmp(line, "id,", 3) == 0)) {
            continue;
        }
        trim_whitespace(line);
        if (line[0] == '\0') continue;

        int id;
        char date[MAX_DATE_LEN] = "";
        char cat[MAX_CAT_LEN] = "";
        double amt = 0.0;
        char desc[MAX_DESC_LEN] = "";

        char *token = strtok(line, ",");
        if (token) id = atoi(token); else continue;

        token = strtok(NULL, ",");
        if (token) strncpy(date, token, sizeof(date) - 1); else continue;

        token = strtok(NULL, ",");
        if (token) strncpy(cat, token, sizeof(cat) - 1); else continue;

        token = strtok(NULL, ",");
        if (token) amt = atof(token); else continue;

        token = strtok(NULL, ",");
        if (token) strncpy(desc, token, sizeof(desc) - 1);

        if (ensure_capacity(list, list->count + 1)) {
            Expense *e = &list->items[list->count++];
            e->id = id;
            strncpy(e->date, date, sizeof(e->date) - 1);
            strncpy(e->category, cat, sizeof(e->category) - 1);
            e->amount = amt;
            strncpy(e->description, desc, sizeof(e->description) - 1);

            if (id >= list->next_id) list->next_id = id + 1;
            loaded++;
        }
    }
    fclose(f);
    return loaded;
}

/* --- Table Display & Analytics --- */

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

typedef struct {
    char name[MAX_CAT_LEN];
    double total;
    size_t count;
} CategoryStat;

void generate_statistics(const ExpenseList *list, const char *export_filepath) {
    if (!list || list->count == 0) {
        printf("\n[ No expenses recorded yet. ]\n");
        return;
    }

    double grand_total = 0.0;
    double max_amt = list->items[0].amount;
    double min_amt = list->items[0].amount;
    const Expense *highest = &list->items[0];
    const Expense *lowest = &list->items[0];

    CategoryStat cats[64];
    size_t cat_count = 0;

    for (size_t i = 0; i < list->count; i++) {
        const Expense *e = &list->items[i];
        grand_total += e->amount;

        if (e->amount > max_amt) { max_amt = e->amount; highest = e; }
        if (e->amount < min_amt) { min_amt = e->amount; lowest = e; }

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

    double average = grand_total / (double)list->count;

    printf("====================================================================\n");
    printf("                     EXPENSE SUMMARY & ANALYTICS                    \n");
    printf("====================================================================\n");
    printf("  Total Records     : %lu\n", (unsigned long)list->count);
    printf("  Total Expenditure : Rs. %.2f\n", grand_total);
    printf("  Average Expense   : Rs. %.2f\n", average);
    printf("  Highest Expense   : Rs. %.2f (#%d - %s [%s]: \"%s\")\n",
           highest->amount, highest->id, highest->date, highest->category, highest->description);
    printf("  Lowest Expense    : Rs. %.2f (#%d - %s [%s]: \"%s\")\n",
           lowest->amount, lowest->id, lowest->date, lowest->category, lowest->description);
    printf("--------------------------------------------------------------------\n");
    printf("CATEGORY BREAKDOWN:\n");
    printf("--------------------------------------------------------------------\n");
    printf("%-22s | %-5s | %-10s | %-6s | %s\n", "Category", "Count", "Total (Rs)", "%", "Visual Distribution");
    printf("-----------------------+-------+------------+--------+----------------------\n");

    for (size_t c = 0; c < cat_count; c++) {
        double pct = (grand_total > 0.0) ? (cats[c].total / grand_total) * 100.0 : 0.0;
        int bar_width = (int)(pct / 5.0);
        if (bar_width > 20) bar_width = 20;

        char bar[32];
        for (int b = 0; b < bar_width; b++) bar[b] = '#';
        for (int b = bar_width; b < 20; b++) bar[b] = ' ';
        bar[20] = '\0';

        printf("%-22s | %-5lu | %10.2f | %5.1f%% | [%s]\n",
               cats[c].name, (unsigned long)cats[c].count, cats[c].total, pct, bar);
    }
    printf("====================================================================\n");

    if (export_filepath && export_filepath[0] != '\0') {
        FILE *f = fopen(export_filepath, "w");
        if (f) {
            fprintf(f, "Total Records: %lu\nTotal Spent: $%.2f\nAverage: $%.2f\n",
                    (unsigned long)list->count, grand_total, average);
            fclose(f);
            printf("\n>> Summary exported to '%s'!\n", export_filepath);
        }
    }
}

/* --- Filtering, Searching & Sorting --- */

void filter_by_category(const ExpenseList *list, const char *category) {
    if (!list || !category) return;
    Expense *matches = (Expense *)malloc(list->count * sizeof(Expense));
    if (!matches && list->count > 0) return;

    size_t count = 0;
    for (size_t i = 0; i < list->count; i++) {
        if (case_insensitive_contains(list->items[i].category, category)) {
            matches[count++] = list->items[i];
        }
    }
    char title[128];
    snprintf(title, sizeof(title), "EXPENSES IN CATEGORY: '%s'", category);
    display_expenses_table(matches, count, title);
    free(matches);
}

void filter_by_date_range(const ExpenseList *list, const char *start_date, const char *end_date) {
    if (!list || !start_date || !end_date) return;
    Expense *matches = (Expense *)malloc(list->count * sizeof(Expense));
    if (!matches && list->count > 0) return;

    size_t count = 0;
    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->items[i].date, start_date) >= 0 && strcmp(list->items[i].date, end_date) <= 0) {
            matches[count++] = list->items[i];
        }
    }
    char title[128];
    snprintf(title, sizeof(title), "EXPENSES BETWEEN %s AND %s", start_date, end_date);
    display_expenses_table(matches, count, title);
    free(matches);
}

void search_expenses(const ExpenseList *list, const char *keyword) {
    if (!list || !keyword) return;
    Expense *matches = (Expense *)malloc(list->count * sizeof(Expense));
    if (!matches && list->count > 0) return;

    size_t count = 0;
    for (size_t i = 0; i < list->count; i++) {
        if (case_insensitive_contains(list->items[i].description, keyword) ||
            case_insensitive_contains(list->items[i].category, keyword)) {
            matches[count++] = list->items[i];
        }
    }
    char title[128];
    snprintf(title, sizeof(title), "SEARCH RESULTS FOR: '%s'", keyword);
    display_expenses_table(matches, count, title);
    free(matches);
}

static int cmp_amt_desc(const void *a, const void *b) {
    double diff = ((const Expense *)b)->amount - ((const Expense *)a)->amount;
    return (diff > 0) - (diff < 0);
}
static int cmp_date_desc(const void *a, const void *b) {
    return strcmp(((const Expense *)b)->date, ((const Expense *)a)->date);
}

/* --- Interactive Menu Handlers --- */

void prompt_category(char *out_cat, size_t size) {
    printf("\nSelect Category:\n");
    for (int i = 0; i < PRESET_CATEGORIES_COUNT; i++) {
        printf("  [%2d] %-22s", i + 1, PRESET_CATEGORIES[i]);
        if ((i + 1) % 2 == 0) printf("\n");
    }
    printf("  [ 0] Custom Category\n");

    while (1) {
        printf("Enter choice (0-%d): ", PRESET_CATEGORIES_COUNT);
        int choice;
        if (get_safe_int(&choice)) {
            if (choice >= 1 && choice <= PRESET_CATEGORIES_COUNT) {
                strncpy(out_cat, PRESET_CATEGORIES[choice - 1], size - 1);
                out_cat[size - 1] = '\0';
                return;
            } else if (choice == 0) {
                printf("Enter custom category: ");
                get_safe_string(out_cat, size);
                trim_whitespace(out_cat);
                if (out_cat[0] != '\0') return;
            }
        }
        printf("Invalid choice! Try again.\n");
    }
}

void prompt_date(char *out_date, size_t size) {
    char today[MAX_DATE_LEN];
    get_current_date(today, sizeof(today));
    while (1) {
        printf("Enter date (YYYY-MM-DD) [Blank or 't' for today: %s]: ", today);
        char buf[32];
        get_safe_string(buf, sizeof(buf));
        trim_whitespace(buf);
        if (buf[0] == '\0' || strcmp(buf, "t") == 0 || strcmp(buf, "T") == 0) {
            strncpy(out_date, today, size - 1);
            out_date[size - 1] = '\0';
            return;
        }
        if (is_valid_date(buf)) {
            strncpy(out_date, buf, size - 1);
            out_date[size - 1] = '\0';
            return;
        }
        printf("Invalid date! Format must be YYYY-MM-DD. Try again.\n");
    }
}

void handle_add(ExpenseList *list) {
    printf("\n--- ADD NEW EXPENSE ---\n");
    char date[MAX_DATE_LEN], cat[MAX_CAT_LEN], desc[MAX_DESC_LEN];
    double amt = 0.0;

    prompt_date(date, sizeof(date));
    prompt_category(cat, sizeof(cat));

    while (1) {
        printf("Enter amount (Rs.): ");
        if (get_safe_double(&amt) && amt > 0.0) break;
        printf("Invalid amount! Must be > 0.\n");
    }

    printf("Enter description: ");
    get_safe_string(desc, sizeof(desc));
    trim_whitespace(desc);

    int id = expense_list_add(list, date, cat, amt, desc);
    if (id > 0) {
        expense_list_save_csv(list, CSV_FILE);
        printf("\n>> Success: Expense #%d added & saved!\n", id);
    }
    pause_prompt();
}

void handle_edit(ExpenseList *list) {
    printf("\n--- EDIT EXPENSE ---\n");
    printf("Enter ID to edit: ");
    int id;
    if (!get_safe_int(&id)) { printf("Invalid ID.\n"); pause_prompt(); return; }

    Expense *e = expense_list_find_by_id(list, id);
    if (!e) { printf("Expense #%d not found.\n", id); pause_prompt(); return; }

    printf("Current: %s | %s | Rs. %.2f | \"%s\"\n", e->date, e->category, e->amount, e->description);
    printf("Leave blank to keep existing values.\n");

    char buf[128];
    char new_date[MAX_DATE_LEN] = "";
    printf("New Date [Blank = keep '%s']: ", e->date);
    get_safe_string(buf, sizeof(buf));
    trim_whitespace(buf);
    if (buf[0] && is_valid_date(buf)) strncpy(new_date, buf, sizeof(new_date) - 1);

    char new_cat[MAX_CAT_LEN] = "";
    printf("Change Category? (y/N): ");
    get_safe_string(buf, sizeof(buf));
    if (buf[0] == 'y' || buf[0] == 'Y') prompt_category(new_cat, sizeof(new_cat));

    double new_amt = -1.0;
    printf("New Amount (Rs.) [Blank = keep Rs. %.2f]: ", e->amount);
    get_safe_string(buf, sizeof(buf));
    trim_whitespace(buf);
    if (buf[0]) {
        double val = atof(buf);
        if (val > 0.0) new_amt = val;
    }

    char new_desc[MAX_DESC_LEN] = "";
    printf("New Description [Blank = keep '%s']: ", e->description);
    get_safe_string(new_desc, sizeof(new_desc));
    trim_whitespace(new_desc);

    expense_list_update(list, id, new_date[0] ? new_date : NULL,
                        new_cat[0] ? new_cat : NULL, new_amt,
                        new_desc[0] ? new_desc : NULL);

    expense_list_save_csv(list, CSV_FILE);
    printf("\n>> Expense #%d updated!\n", id);
    pause_prompt();
}

void handle_delete(ExpenseList *list) {
    printf("\n--- DELETE EXPENSE ---\n");
    printf("Enter ID to delete: ");
    int id;
    if (!get_safe_int(&id)) { printf("Invalid ID.\n"); pause_prompt(); return; }

    Expense *e = expense_list_find_by_id(list, id);
    if (!e) { printf("Expense #%d not found.\n", id); pause_prompt(); return; }

    printf("Are you sure you want to delete Expense #%d (Rs. %.2f - %s)? (y/N): ", e->id, e->amount, e->description);
    char confirm[16];
    get_safe_string(confirm, sizeof(confirm));
    if (confirm[0] == 'y' || confirm[0] == 'Y') {
        expense_list_delete(list, id);
        expense_list_save_csv(list, CSV_FILE);
        printf("\n>> Expense #%d deleted.\n", id);
    } else {
        printf("\nDeletion cancelled.\n");
    }
    pause_prompt();
}

int main(void) {
    ExpenseList list;
    expense_list_init(&list);
    expense_list_load_csv(&list, CSV_FILE);

    int running = 1;
    while (running) {
        printf("\n======================================================\n");
        printf("           PERSONAL EXPENSE TRACKER SYSTEM            \n");
        printf("======================================================\n");
        printf("  [1] Add New Expense\n");
        printf("  [2] View All Expenses\n");
        printf("  [3] Filter by Category\n");
        printf("  [4] Filter by Date Range\n");
        printf("  [5] Search by Keyword\n");
        printf("  [6] View Expense Analytics & Statistics\n");
        printf("  [7] Edit an Expense\n");
        printf("  [8] Delete an Expense\n");
        printf("  [9] Sort by Amount (Highest First)\n");
        printf("  [10] Sort by Date (Newest First)\n");
        printf("  [0] Save & Exit\n");
        printf("======================================================\n");
        printf("Enter your choice (0-10): ");

        int choice;
        if (!get_safe_int(&choice)) {
            printf("Invalid input! Please enter a number.\n");
            pause_prompt();
            continue;
        }

        switch (choice) {
            case 1: handle_add(&list); break;
            case 2: display_expenses_table(list.items, list.count, "ALL RECORDED EXPENSES"); pause_prompt(); break;
            case 3: {
                char cat[MAX_CAT_LEN];
                printf("Enter category name to filter: ");
                get_safe_string(cat, sizeof(cat));
                trim_whitespace(cat);
                filter_by_category(&list, cat);
                pause_prompt();
                break;
            }
            case 4: {
                char s[MAX_DATE_LEN], e[MAX_DATE_LEN];
                printf("Enter Start Date:\n"); prompt_date(s, sizeof(s));
                printf("Enter End Date:\n"); prompt_date(e, sizeof(e));
                filter_by_date_range(&list, s, e);
                pause_prompt();
                break;
            }
            case 5: {
                char kw[MAX_DESC_LEN];
                printf("Enter keyword to search: ");
                get_safe_string(kw, sizeof(kw));
                trim_whitespace(kw);
                search_expenses(&list, kw);
                pause_prompt();
                break;
            }
            case 6: generate_statistics(&list, "expense_report.txt"); pause_prompt(); break;
            case 7: handle_edit(&list); break;
            case 8: handle_delete(&list); break;
            case 9:
                qsort(list.items, list.count, sizeof(Expense), cmp_amt_desc);
                expense_list_save_csv(&list, CSV_FILE);
                display_expenses_table(list.items, list.count, "EXPENSES SORTED BY AMOUNT (HIGHEST FIRST)");
                pause_prompt();
                break;
            case 10:
                qsort(list.items, list.count, sizeof(Expense), cmp_date_desc);
                expense_list_save_csv(&list, CSV_FILE);
                display_expenses_table(list.items, list.count, "EXPENSES SORTED BY DATE (NEWEST FIRST)");
                pause_prompt();
                break;
            case 0:
                expense_list_save_csv(&list, CSV_FILE);
                printf("\nAll data saved to '%s'. Goodbye!\n\n", CSV_FILE);
                running = 0;
                break;
            default:
                printf("Invalid selection!\n");
                pause_prompt();
                break;
        }
    }

    expense_list_free(&list);
    return 0;
}
