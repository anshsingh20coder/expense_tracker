# SpendWise — Personal Expense Tracker System (C++ Backend)

A complete personal finance tracker featuring a sleek, responsive modern Web Interface (HTML5, CSS3, JavaScript) powered by a high-performance native **C++ HTTP backend server** (C++11 with Object-Oriented Design and Windows Sockets / Winsock2). It also includes a standalone command-line interface (CLI). Both share the same persistent CSV database (`expenses.csv`).

---

## 🌟 Key Features

### 🌐 Web Dashboard (`http://localhost:8080`)
- **Modern Clean White Theme**: Professional, high-contrast fintech aesthetic with crisp borders and clean typography.
- **Indian Rupee (₹) Currency**: Full Indian Rupee denomination formatting across all cards, transaction tables, edit modals, and category breakdowns.
- **Real-Time KPI Cards**:
  - Total Spending (dynamic currency counter).
  - Total Transactions count.
  - Average expense per transaction.
  - Highest and lowest expense indicators with descriptions and dates.
- **Visual Category Breakdown**:
  - Horizontal progress bars with percentage of total spending and monetary amounts.
  - Color-coded badges for standard categories + custom categories.
- **Interactive Expense Log**:
  - Live instant search (search descriptions, categories, or dates).
  - Category filter dropdown.
  - Sorting (Newest, Oldest, Highest amount, Lowest amount).
  - In-place Edit modal to update any field seamlessly.
  - Delete action with confirmation.
- **Quick-Add Expense Form**:
  - Auto-selects today's date.
  - Category selector with custom category support.
  - Instant toast alerts upon submission.

### 🖥️ Native C++ HTTP Backend Server
- Written in modern C++ (C++11) with clean Object-Oriented Architecture (`Expense`, `ExpenseManager`, `HttpServer`, RAII socket management). Zero external dependencies (no Node.js, Python, or external servers needed).
- Serves static assets (`index.html`, `style.css`, `app.js`).
- RESTful JSON API:
  - `GET /api/expenses` — Fetch all recorded expenses.
  - `POST /api/expenses` — Create a new expense.
  - `POST /api/expenses/update` — Update an existing expense.
  - `POST /api/expenses/delete` — Delete an expense.
  - `GET /api/stats` — Real-time calculated analytics.
- Instant synchronization with `expenses.csv`.

### 💻 Classic CLI Mode
- Standalone C++ command-line terminal tool with interactive menus, formatted tables, and text report export.

---

## 📁 Directory Structure

```
expense_tracker/
│
├── web/
│   ├── index.html                  # Modern dashboard UI
│   ├── style.css                   # Clean white theme styling
│   └── app.js                      # Client-side API integration & DOM updates
│
├── include/
│   ├── expense_manager.hpp         # C++ Expense & ExpenseManager class definitions
│   ├── server.hpp                  # C++ HttpRequest & HttpServer definitions
│   ├── expense.h                   # Legacy C headers (retained for backward compatibility)
│   ├── utils.h
│   └── server.h
│
├── src/
│   ├── main.cpp                    # C++ CLI application entrypoint
│   ├── server_main.cpp             # C++ Web server entrypoint
│   ├── server.cpp                  # C++ HTTP server & Winsock2 request router
│   ├── expense_manager.cpp         # C++ OOP business logic & CSV persistence
│   ├── main.c                      # Legacy C source files (retained)
│   ├── server.c
│   ├── expense.c
│   └── utils.c
│
├── standalone_expense_tracker.cpp  # Single-file self-contained C++ application
├── build.bat                       # Compiles C++ CLI executable (g++)
├── build_server.bat                # Compiles C++ Web Server executable (g++)
├── run_web.bat                     # One-click launcher: starts server & opens browser
├── share_online.bat                # One-click public sharing via Cloudflare Tunnel
├── Makefile                        # Multi-target Makefile (g++)
├── expenses.csv                    # Shared CSV database
├── cloudflared.exe                 # Persistent Cloudflare Tunnel client
└── README.md                       # Documentation
```

---

## 🚀 How to Run

### 1. Launch the Web Application (Local)
Simply double-click or run from terminal:
```cmd
run_web.bat
```
This starts `expense_server.exe` on port 8080 and opens:
👉 **[http://localhost:8080](http://localhost:8080)**

### 2. Share Publicly Online (Cloudflare Tunnel)
To share the website over the internet without timeouts:
Double-click:
```cmd
share_online.bat
```
This launches a persistent, high-speed **Cloudflare Tunnel** (`.trycloudflare.com`) powered by `cloudflared.exe`.

### 3. Run the CLI Version
```cmd
build.bat
expense_tracker.exe
```

### 4. Compile with Makefile or G++
To compile both the web server and CLI:
```bash
make
```
Or manually:
```bash
# Compile C++ Web Server
g++ -Wall -Wextra -std=c++11 -Iinclude src/server_main.cpp src/server.cpp src/expense_manager.cpp -lws2_32 -o expense_server.exe

# Compile C++ CLI Tool
g++ -Wall -Wextra -std=c++11 -Iinclude src/main.cpp src/expense_manager.cpp -o expense_tracker.exe
```

---

## 💾 CSV Data Persistence (`expenses.csv`)

All data added or modified through either the Web UI or the CLI is instantly saved in human-readable CSV format:
```csv
ID,Date,Category,Amount,Description
1,2026-10-01,Food,450.00,Dinner with friends at cafe
2,2026-10-02,Transportation,120.00,Metro recharge card
3,2026-10-03,Shopping,2499.00,Sports shoes from store
4,2026-10-04,Groceries,350.00,Fresh vegetables and milk
5,2026-10-05,Entertainment,420.00,Cinema movie ticket
```
You can open this file anytime in Microsoft Excel, Google Sheets, or any text editor.
