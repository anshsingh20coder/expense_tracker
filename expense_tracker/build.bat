@echo off
echo ==============================================
echo   Compiling Expense Tracker System (C++ CLI)
echo ==============================================

g++ -Wall -Wextra -std=c++11 -Iinclude src/main.cpp src/expense_manager.cpp -o expense_tracker.exe

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Compilation finished: expense_tracker.exe created!
    echo Run the program with: .\expense_tracker.exe
) else (
    echo [ERROR] Compilation failed. Please check g++ output.
)
