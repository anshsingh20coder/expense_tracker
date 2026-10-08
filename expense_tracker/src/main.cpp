#include "expense_manager.hpp"
#include <iostream>
#include <string>
#include <ctime>

static std::string getTodayDate() {
    time_t raw = time(nullptr);
    struct tm* info = localtime(&raw);
    char buf[32];
    if (info) {
        strftime(buf, sizeof(buf), "%Y-%m-%d", info);
        return std::string(buf);
    }
    return "2026-01-01";
}

static void pausePrompt() {
    std::cout << "\nPress [Enter] to continue...";
    std::string dummy;
    std::getline(std::cin, dummy);
}

int main() {
    ExpenseManager manager("expenses.csv");

    bool running = true;
    while (running) {
        std::cout << "\n======================================================\n";
        std::cout << "        PERSONAL EXPENSE TRACKER SYSTEM (C++)         \n";
        std::cout << "======================================================\n";
        std::cout << "  [1] Add New Expense\n";
        std::cout << "  [2] View All Expenses\n";
        std::cout << "  [3] Filter by Category\n";
        std::cout << "  [4] Filter by Date Range\n";
        std::cout << "  [5] Search by Keyword\n";
        std::cout << "  [6] View Expense Analytics & Statistics\n";
        std::cout << "  [7] Edit an Expense\n";
        std::cout << "  [8] Delete an Expense\n";
        std::cout << "  [9] Sort by Amount (Highest First)\n";
        std::cout << "  [10] Sort by Date (Newest First)\n";
        std::cout << "  [0] Save & Exit\n";
        std::cout << "======================================================\n";
        std::cout << "Enter your choice (0-10): ";

        std::string choiceStr;
        if (!std::getline(std::cin, choiceStr)) break;
        int choice = -1;
        try {
            choice = std::stoi(choiceStr);
        } catch (...) {
            std::cout << "Invalid input! Please enter a number.\n";
            pausePrompt();
            continue;
        }

        switch (choice) {
            case 1: {
                std::cout << "\n--- ADD NEW EXPENSE ---\n";
                std::string today = getTodayDate();
                std::cout << "Enter date (YYYY-MM-DD) [Blank for today: " << today << "]: ";
                std::string date;
                std::getline(std::cin, date);
                if (date.empty()) date = today;

                std::cout << "Enter category (e.g. Food & Dining, Transportation, Shopping): ";
                std::string category;
                std::getline(std::cin, category);
                if (category.empty()) category = "Other";

                double amount = 0.0;
                while (true) {
                    std::cout << "Enter amount (Rs.): ";
                    std::string amtStr;
                    std::getline(std::cin, amtStr);
                    try {
                        amount = std::stod(amtStr);
                        if (amount > 0.0) break;
                    } catch (...) {}
                    std::cout << "Invalid amount! Must be greater than 0.\n";
                }

                std::cout << "Enter description / note: ";
                std::string desc;
                std::getline(std::cin, desc);

                int id = manager.addExpense(date, category, amount, desc);
                if (id > 0) {
                    std::cout << "\n>> Success: Expense #" << id << " added and saved!\n";
                }
                pausePrompt();
                break;
            }
            case 2:
                manager.displayTable(manager.getAllExpenses(), "ALL RECORDED EXPENSES");
                pausePrompt();
                break;
            case 3: {
                std::cout << "Enter category name or keyword: ";
                std::string cat;
                std::getline(std::cin, cat);
                auto res = manager.filterByCategory(cat);
                manager.displayTable(res, "EXPENSES IN CATEGORY: '" + cat + "'");
                pausePrompt();
                break;
            }
            case 4: {
                std::cout << "Enter Start Date (YYYY-MM-DD): ";
                std::string s; std::getline(std::cin, s);
                std::cout << "Enter End Date (YYYY-MM-DD): ";
                std::string e; std::getline(std::cin, e);
                auto res = manager.filterByDateRange(s, e);
                manager.displayTable(res, "EXPENSES BETWEEN " + s + " AND " + e);
                pausePrompt();
                break;
            }
            case 5: {
                std::cout << "Enter keyword to search: ";
                std::string kw;
                std::getline(std::cin, kw);
                auto res = manager.search(kw);
                manager.displayTable(res, "SEARCH RESULTS FOR: '" + kw + "'");
                pausePrompt();
                break;
            }
            case 6:
                manager.printReport(std::cout);
                pausePrompt();
                break;
            case 7: {
                std::cout << "Enter ID to edit: ";
                std::string idStr;
                std::getline(std::cin, idStr);
                try {
                    int id = std::stoi(idStr);
                    const Expense* exp = manager.findById(id);
                    if (!exp) {
                        std::cout << "Expense #" << id << " not found!\n";
                    } else {
                        std::cout << "Current: " << exp->date << " | " << exp->category
                                  << " | Rs. " << exp->amount << " | \"" << exp->description << "\"\n";
                        std::cout << "Leave blank to keep existing value.\n";

                        std::cout << "New Date [Blank = keep '" << exp->date << "']: ";
                        std::string newDate; std::getline(std::cin, newDate);

                        std::cout << "New Category [Blank = keep '" << exp->category << "']: ";
                        std::string newCat; std::getline(std::cin, newCat);

                        double newAmt = -1.0;
                        std::cout << "New Amount (Rs.) [Blank = keep Rs. " << exp->amount << "]: ";
                        std::string amtStr; std::getline(std::cin, amtStr);
                        if (!amtStr.empty()) {
                            try { newAmt = std::stod(amtStr); } catch (...) {}
                        }

                        std::cout << "New Description [Blank = keep '" << exp->description << "']: ";
                        std::string newDesc; std::getline(std::cin, newDesc);

                        if (manager.updateExpense(id, newDate, newCat, newAmt, newDesc)) {
                            std::cout << "\n>> Success: Expense #" << id << " updated!\n";
                        }
                    }
                } catch (...) {
                    std::cout << "Invalid ID.\n";
                }
                pausePrompt();
                break;
            }
            case 8: {
                std::cout << "Enter ID to delete: ";
                std::string idStr;
                std::getline(std::cin, idStr);
                try {
                    int id = std::stoi(idStr);
                    const Expense* exp = manager.findById(id);
                    if (!exp) {
                        std::cout << "Expense #" << id << " not found!\n";
                    } else {
                        std::cout << "Are you sure you want to delete Expense #" << id
                                  << " (Rs. " << exp->amount << " - " << exp->description << ")? (y/N): ";
                        std::string conf;
                        std::getline(std::cin, conf);
                        if (conf == "y" || conf == "Y") {
                            manager.deleteExpense(id);
                            std::cout << "\n>> Expense #" << id << " deleted.\n";
                        } else {
                            std::cout << "\nDeletion cancelled.\n";
                        }
                    }
                } catch (...) {
                    std::cout << "Invalid ID.\n";
                }
                pausePrompt();
                break;
            }
            case 9:
                manager.sortByAmount(true);
                manager.displayTable(manager.getAllExpenses(), "EXPENSES SORTED BY AMOUNT (HIGHEST FIRST)");
                pausePrompt();
                break;
            case 10:
                manager.sortByDate(true);
                manager.displayTable(manager.getAllExpenses(), "EXPENSES SORTED BY DATE (NEWEST FIRST)");
                pausePrompt();
                break;
            case 0:
                std::cout << "\nAll data saved to 'expenses.csv'. Thank you for using Expense Tracker C++!\n\n";
                running = false;
                break;
            default:
                std::cout << "Invalid choice!\n";
                pausePrompt();
                break;
        }
    }

    return 0;
}
