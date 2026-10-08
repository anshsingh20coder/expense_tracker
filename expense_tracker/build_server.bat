@echo off
echo =====================================================
echo   Compiling Expense Tracker Web Server (C++ Backend)
echo =====================================================

g++ -Wall -Wextra -std=c++11 -Iinclude src/server_main.cpp src/server.cpp src/expense_manager.cpp -lws2_32 -o expense_server.exe

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Compilation finished: expense_server.exe created!
    echo Run the server with: .\expense_server.exe
) else (
    echo [ERROR] Compilation failed. Please check g++ output.
)
