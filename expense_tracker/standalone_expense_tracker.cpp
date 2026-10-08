/**
 * ============================================================================
 * Project      : Personal Expense Tracker System (C++)
 * Language     : C++ (Standard C++11 / C++14)
 * Description  : Complete self-contained Object-Oriented Expense Tracker System.
 * Features     :
 *   - Object-Oriented design with Expense & ExpenseManager classes
 *   - Auto-incremented ID generation & date handling
 *   - Formatted tabular display with running totals in Rupees (₹ / Rs.)
 *   - Category, date range, and keyword filtering
 *   - Statistical reports with ASCII category distribution bars
 *   - Full CRUD: Add, View, Edit, Delete with confirmations
 *   - Sorting by amount (highest first) and date (newest first)
 *   - Automatic CSV persistence (expenses.csv)
 *
 * Compilation  : g++ -std=c++11 -Wall -Wextra standalone_expense_tracker.cpp -o expense_tracker.exe
 * Execution    : .\expense_tracker.exe
 * ============================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <cctype>

class Expense {
public:
    int id;
    std::string date;
    std::string category;
    double amount;
    std::string description;

    Expense() : id(0), amount(0.0) {}
    Expense(int id, const std::string& date, const std::string& category, double amount, const std::string& description)
        : id(id), date(date), category(category), amount(amount), description(description) {}

    std::string toCsv() const {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2);
        ss << id << "," << date << "," << category << "," << amount << "," << description;
        return ss.str();
    }
};

class ExpenseManager {
private:
    std::vector<Expense> expenses;
    int nextId;
    std::string csvPath;

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    static bool containsIgnoreCase(const std::string& str, const std::string& query) {
        if (query.empty()) return true;
        auto it = std::search(
            str.begin(), str.end(),
            query.begin(), query.end(),
            [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
        );
        return (it != str.end());
    }

public:
    ExpenseManager(const std::string& path = "expenses.csv") : nextId(1), csvPath(path) {
        loadFromCsv();
    }

    bool loadFromCsv() {
        expenses.clear();
        nextId = 1;

        std::ifstream file(csvPath);
        if (!file.is_open()) return false;

        std::string line;
        bool isHeader = true;

        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty()) continue;

            if (isHeader) {
                isHeader = false;
                if (line.substr(0, 3) == "ID," || line.substr(0, 3) == "id,") continue;
            }

            std::stringstream ss(line);
            std::string item;
            std::vector<std::string> fields;
            while (std::getline(ss, item, ',')) {
                fields.push_back(trim(item));
            }

            if (fields.size() >= 4) {
                int id = std::stoi(fields[0]);
                std::string date = fields[1];
                std::string category = fields[2];
                double amount = std::stod(fields[3]);
                std::string desc = (fields.size() >= 5) ? fields[4] : "";

                expenses.emplace_back(id, date, category, amount, desc);
                if (id >= nextId) nextId = id + 1;
            }
        }
        return true;
    }

    bool saveToCsv() const {
        std::ofstream file(csvPath);
        if (!file.is_open()) return false;

        file << "ID,Date,Category,Amount,Description\n";
        for (const auto& e : expenses) {
            file << e.toCsv() << "\n";
        }
        return true;
    }

    int addExpense(const std::string& date, const std::string& category, double amount, const std::string& description) {
        if (amount <= 0.0) return -1;
        int newId = nextId++;
        expenses.emplace_back(newId, date, category, amount, description);
        saveToCsv();
        return newId;
    }

    bool updateExpense(int id, const std::string& date, const std::string& category, double amount, const std::string& description) {
        for (auto& e : expenses) {
            if (e.id == id) {
                if (!date.empty()) e.date = date;
                if (!category.empty()) e.category = category;
                if (amount > 0.0) e.amount = amount;
                if (!description.empty()) e.description = description;
                saveToCsv();
                return true;
            }
        }
        return false;
    }

    bool deleteExpense(int id) {
        auto it = std::remove_if(expenses.begin(), expenses.end(), [id](const Expense& e) {
            return e.id == id;
        });
        if (it != expenses.end()) {
            expenses.erase(it, expenses.end());
            saveToCsv();
            return true;
        }
        return false;
    }

    const std::vector<Expense>& getAllExpenses() const { return expenses; }

    const Expense* findById(int id) const {
        for (const auto& e : expenses) {
            if (e.id == id) return &e;
        }
        return nullptr;
    }

    std::vector<Expense> filterByCategory(const std::string& cat) const {
        std::vector<Expense> res;
        for (const auto& e : expenses) {
            if (containsIgnoreCase(e.category, cat)) res.push_back(e);
        }
        return res;
    }

    std::vector<Expense> filterByDateRange(const std::string& start, const std::string& end) const {
        std::vector<Expense> res;
        for (const auto& e : expenses) {
            if (e.date >= start && e.date <= end) res.push_back(e);
        }
        return res;
    }

    std::vector<Expense> search(const std::string& keyword) const {
        std::vector<Expense> res;
        for (const auto& e : expenses) {
            if (containsIgnoreCase(e.description, keyword) || containsIgnoreCase(e.category, keyword) || containsIgnoreCase(e.date, keyword)) {
                res.push_back(e);
            }
        }
        return res;
    }

    void displayTable(const std::vector<Expense>& items, const std::string& title = "") const {
        if (!title.empty()) std::cout << "\n=== " << title << " ===\n";
        if (items.empty()) {
            std::cout << "\n[ No expenses found matching your criteria. ]\n";
            return;
        }

        std::cout << "+------+------------+---------------------+------------+--------------------------------+\n";
        std::cout << "|  ID  |    Date    | Category            |   Amount   | Description                    |\n";
        std::cout << "+------+------------+---------------------+------------+--------------------------------+\n";

        double total = 0.0;
        for (const auto& e : items) {
            total += e.amount;
            std::string desc = e.description;
            if (desc.length() > 30) desc = desc.substr(0, 27) + "...";

            std::cout << "| " << std::setw(4) << e.id << " | "
                      << std::setw(10) << e.date << " | "
                      << std::left << std::setw(19) << e.category << " | "
                      << std::right << std::fixed << std::setprecision(2) << std::setw(10) << e.amount << " | "
                      << std::left << std::setw(30) << desc << " |\n";
        }

        std::cout << "+------+------------+---------------------+------------+--------------------------------+\n";
        std::cout << "| Total: " << std::left << std::setw(4) << items.size() << " item(s)"
                  << std::setw(20) << " " << " | "
                  << std::right << std::fixed << std::setprecision(2) << std::setw(10) << total << " | "
                  << std::setw(32) << " " << "|\n";
        std::cout << "+------+------------+---------------------+------------+--------------------------------+\n";
    }

    void printReport(std::ostream& out) const {
        if (expenses.empty()) {
            out << "\n[ No expenses recorded yet. ]\n";
            return;
        }

        double total = 0.0;
        std::map<std::string, std::pair<double, size_t>> catMap;
        const Expense* highest = &expenses[0];
        const Expense* lowest = &expenses[0];

        for (const auto& e : expenses) {
            total += e.amount;
            catMap[e.category].first += e.amount;
            catMap[e.category].second += 1;

            if (e.amount > highest->amount) highest = &e;
            if (e.amount < lowest->amount) lowest = &e;
        }

        double avg = total / expenses.size();

        out << std::fixed << std::setprecision(2);
        out << "====================================================================\n";
        out << "                     EXPENSE SUMMARY & ANALYTICS                    \n";
        out << "====================================================================\n";
        out << "  Total Records     : " << expenses.size() << "\n";
        out << "  Total Expenditure : Rs. " << total << "\n";
        out << "  Average Expense   : Rs. " << avg << "\n";
        out << "  Highest Expense   : Rs. " << highest->amount << " (#" << highest->id << " - " << highest->date
            << " [" << highest->category << "]: \"" << highest->description << "\")\n";
        out << "  Lowest Expense    : Rs. " << lowest->amount << " (#" << lowest->id << " - " << lowest->date
            << " [" << lowest->category << "]: \"" << lowest->description << "\")\n";
        out << "--------------------------------------------------------------------\n";
        out << "CATEGORY BREAKDOWN:\n";
        out << "--------------------------------------------------------------------\n";
        out << std::left << std::setw(22) << "Category" << " | "
            << std::setw(5) << "Count" << " | "
            << std::setw(10) << "Total (Rs)" << " | "
            << std::setw(6) << "%" << " | "
            << "Visual Distribution\n";
        out << "-----------------------+-------+------------+--------+----------------------\n";

        for (const auto& p : catMap) {
            double catTotal = p.second.first;
            size_t count = p.second.second;
            double pct = (total > 0.0) ? (catTotal / total * 100.0) : 0.0;
            int barWidth = std::min(20, static_cast<int>(pct / 5.0));

            std::string bar(barWidth, '#');
            bar.append(20 - barWidth, ' ');

            out << std::left << std::setw(22) << p.first << " | "
                << std::setw(5) << count << " | "
                << std::right << std::setw(10) << catTotal << " | "
                << std::setw(5) << pct << "% | ["
                << bar << "]\n";
        }
        out << "====================================================================\n";
    }

    void sortByAmount(bool descending) {
        std::sort(expenses.begin(), expenses.end(), [descending](const Expense& a, const Expense& b) {
            return descending ? (a.amount > b.amount) : (a.amount < b.amount);
        });
        saveToCsv();
    }

    void sortByDate(bool descending) {
        std::sort(expenses.begin(), expenses.end(), [descending](const Expense& a, const Expense& b) {
            return descending ? (a.date > b.date) : (a.date < b.date);
        });
        saveToCsv();
    }
};

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
