#include "expense_manager.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

static std::string escapeJson(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        if (c == '"') ss << "\\\"";
        else if (c == '\\') ss << "\\\\";
        else if (c == '\n') ss << "\\n";
        else if (c == '\r') ss << "\\r";
        else if (c == '\t') ss << "\\t";
        else ss << c;
    }
    return ss.str();
}

static std::string escapeCsv(const std::string& input) {
    if (input.find(',') != std::string::npos || input.find('"') != std::string::npos ||
        input.find('\n') != std::string::npos || input.find('\r') != std::string::npos) {
        std::ostringstream ss;
        ss << '"';
        for (char c : input) {
            if (c == '"') ss << "\"\"";
            else ss << c;
        }
        ss << '"';
        return ss.str();
    }
    return input;
}

Expense::Expense() : id(0), amount(0.0) {}

Expense::Expense(int id, const std::string& date, const std::string& category, double amount, const std::string& description)
    : id(id), date(date), category(category), amount(amount), description(description) {}

std::string Expense::toJson() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "{\"id\": " << id
       << ", \"date\": \"" << escapeJson(date) << "\""
       << ", \"category\": \"" << escapeJson(category) << "\""
       << ", \"amount\": " << amount
       << ", \"description\": \"" << escapeJson(description) << "\"}";
    return ss.str();
}

std::string Expense::toCsv() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << id << "," << date << "," << escapeCsv(category) << "," << amount << "," << escapeCsv(description);
    return ss.str();
}

ExpenseManager::ExpenseManager(const std::string& path) : nextId(1), csvPath(path) {
    loadFromCsv();
}

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool ExpenseManager::loadFromCsv() {
    expenses.clear();
    nextId = 1;

    std::ifstream file(csvPath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    bool isHeader = true;

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty()) continue;

        if (isHeader) {
            isHeader = false;
            if (line.substr(0, 3) == "ID," || line.substr(0, 3) == "id,") {
                continue;
            }
        }

        // CSV Parser supporting quotes
        std::vector<std::string> fields;
        std::string current;
        bool inQuotes = false;

        for (size_t i = 0; i < line.length(); ++i) {
            char c = line[i];
            if (c == '"') {
                if (inQuotes && i + 1 < line.length() && line[i + 1] == '"') {
                    current += '"';
                    i++;
                } else {
                    inQuotes = !inQuotes;
                }
            } else if (c == ',' && !inQuotes) {
                fields.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        }
        fields.push_back(current);

        if (fields.size() >= 4) {
            int id = std::stoi(trim(fields[0]));
            std::string date = trim(fields[1]);
            std::string category = trim(fields[2]);
            double amount = std::stod(trim(fields[3]));
            std::string desc = (fields.size() >= 5) ? trim(fields[4]) : "";

            expenses.emplace_back(id, date, category, amount, desc);
            if (id >= nextId) nextId = id + 1;
        }
    }

    file.close();
    return true;
}

bool ExpenseManager::saveToCsv() const {
    std::ofstream file(csvPath);
    if (!file.is_open()) return false;

    file << "ID,Date,Category,Amount,Description\n";
    for (const auto& e : expenses) {
        file << e.toCsv() << "\n";
    }
    file.close();
    return true;
}

int ExpenseManager::addExpense(const std::string& date, const std::string& category, double amount, const std::string& description) {
    if (amount <= 0.0) return -1;

    int newId = nextId++;
    expenses.emplace_back(newId, date, category, amount, description);
    saveToCsv();
    return newId;
}

bool ExpenseManager::updateExpense(int id, const std::string& date, const std::string& category, double amount, const std::string& description) {
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

bool ExpenseManager::deleteExpense(int id) {
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

const std::vector<Expense>& ExpenseManager::getAllExpenses() const {
    return expenses;
}

const Expense* ExpenseManager::findById(int id) const {
    for (const auto& e : expenses) {
        if (e.id == id) return &e;
    }
    return nullptr;
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

std::vector<Expense> ExpenseManager::filterByCategory(const std::string& cat) const {
    std::vector<Expense> results;
    for (const auto& e : expenses) {
        if (containsIgnoreCase(e.category, cat)) results.push_back(e);
    }
    return results;
}

std::vector<Expense> ExpenseManager::filterByDateRange(const std::string& start, const std::string& end) const {
    std::vector<Expense> results;
    for (const auto& e : expenses) {
        if (e.date >= start && e.date <= end) results.push_back(e);
    }
    return results;
}

std::vector<Expense> ExpenseManager::search(const std::string& keyword) const {
    std::vector<Expense> results;
    for (const auto& e : expenses) {
        if (containsIgnoreCase(e.description, keyword) || containsIgnoreCase(e.category, keyword) || containsIgnoreCase(e.date, keyword)) {
            results.push_back(e);
        }
    }
    return results;
}

std::string ExpenseManager::toJson() const {
    std::ostringstream ss;
    ss << "[\n";
    for (size_t i = 0; i < expenses.size(); ++i) {
        ss << "  " << expenses[i].toJson();
        if (i + 1 < expenses.size()) ss << ",";
        ss << "\n";
    }
    ss << "]";
    return ss.str();
}

std::string ExpenseManager::statsToJson() const {
    double total = 0.0;
    std::map<std::string, std::pair<double, size_t>> catMap;
    const Expense* highest = nullptr;
    const Expense* lowest = nullptr;

    for (const auto& e : expenses) {
        total += e.amount;
        catMap[e.category].first += e.amount;
        catMap[e.category].second += 1;

        if (!highest || e.amount > highest->amount) highest = &e;
        if (!lowest || e.amount < lowest->amount) lowest = &e;
    }

    double average = expenses.empty() ? 0.0 : (total / expenses.size());

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "{\n"
       << "  \"count\": " << expenses.size() << ",\n"
       << "  \"total\": " << total << ",\n"
       << "  \"average\": " << average << ",\n"
       << "  \"categories\": [\n";

    size_t idx = 0;
    for (const auto& pair : catMap) {
        double catTotal = pair.second.first;
        size_t catCount = pair.second.second;
        double pct = (total > 0.0) ? (catTotal / total * 100.0) : 0.0;

        ss << "    {\"name\": \"" << escapeJson(pair.first) << "\""
           << ", \"total\": " << catTotal
           << ", \"count\": " << catCount
           << ", \"percentage\": " << pct << "}";

        if (++idx < catMap.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ]";

    if (highest) {
        ss << ",\n  \"highest\": " << highest->toJson();
    } else {
        ss << ",\n  \"highest\": null";
    }

    if (lowest) {
        ss << ",\n  \"lowest\": " << lowest->toJson();
    } else {
        ss << ",\n  \"lowest\": null";
    }

    ss << "\n}\n";
    return ss.str();
}

void ExpenseManager::displayTable(const std::vector<Expense>& items, const std::string& title) const {
    if (!title.empty()) {
        std::cout << "\n=== " << title << " ===\n";
    }

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

void ExpenseManager::printReport(std::ostream& out) const {
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

void ExpenseManager::sortByAmount(bool descending) {
    std::sort(expenses.begin(), expenses.end(), [descending](const Expense& a, const Expense& b) {
        return descending ? (a.amount > b.amount) : (a.amount < b.amount);
    });
    saveToCsv();
}

void ExpenseManager::sortByDate(bool descending) {
    std::sort(expenses.begin(), expenses.end(), [descending](const Expense& a, const Expense& b) {
        return descending ? (a.date > b.date) : (a.date < b.date);
    });
    saveToCsv();
}
