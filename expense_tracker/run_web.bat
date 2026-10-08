@echo off
echo ===================================================
echo    SpendWise (C++ Backend + Clean White UI)
echo ===================================================

if not exist expense_server.exe (
    echo Compiling C++ web server first...
    call build_server.bat
)

echo Starting C++ Web Server on http://localhost:8080 ...
start "" http://localhost:8080
.\expense_server.exe
