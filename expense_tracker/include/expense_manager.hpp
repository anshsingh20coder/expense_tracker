#ifndef EXPENSE_MANAGER_HPP
#define EXPENSE_MANAGER_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>

class Expense {
public:
    int id;
    std::string date;
    std::string category;
    double amount;
    std::string description;

    Expense();
    Expense(int id, const std::string& date, const std::string& category, double amount, const std::string& description);

    std::string toJson() const;
    std::string toCsv() const;
};

struct CategoryStat {
    std::string name;
    double total;
    size_t count;
    double percentage;
};

class ExpenseManager {
private:
    std::vector<Expense> expenses;
    int nextId;
    std::string csvPath;

public:
    ExpenseManager(const std::string& path = "expenses.csv");

    bool loadFromCsv();
    bool saveToCsv() const;

    int addExpense(const std::string& date, const std::string& category, double amount, const std::string& description);
    bool updateExpense(int id, const std::string& date, const std::string& category, double amount, const std::string& description);
    bool deleteExpense(int id);

    const std::vector<Expense>& getAllExpenses() const;
    const Expense* findById(int id) const;

    std::vector<Expense> filterByCategory(const std::string& cat) const;
    std::vector<Expense> filterByDateRange(const std::string& start, const std::string& end) const;
    std::vector<Expense> search(const std::string& keyword) const;

    std::string toJson() const;
    std::string statsToJson() const;

    void displayTable(const std::vector<Expense>& items, const std::string& title = "") const;
    void printReport(std::ostream& out) const;

    void sortByAmount(bool descending = true);
    void sortByDate(bool descending = true);

    size_t getCount() const { return expenses.size(); }
};

#endif // EXPENSE_MANAGER_HPP
