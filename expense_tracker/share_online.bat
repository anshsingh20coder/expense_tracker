@echo off
echo ==========================================================
echo    SpendWise - Launching Cloudflare Tunnel (Stable)
echo ==========================================================

tasklist /FI "IMAGENAME eq expense_server.exe" 2>NUL | find /I /N "expense_server.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo [OK] C++ Backend Server is already running.
) else (
    echo Starting C++ Backend Server in background...
    start "" /B .\expense_server.exe
    timeout /t 2 >nul
)

echo.
echo Starting secure Cloudflare Tunnel (no timeouts)...
echo Look for the 'https://...trycloudflare.com' link below:
echo.

.\cloudflared.exe tunnel --url http://127.0.0.1:8080 --no-autoupdate
pause
