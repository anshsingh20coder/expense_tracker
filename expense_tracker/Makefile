CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -Iinclude
CLI_SRC = src/main.cpp src/expense_manager.cpp
SERVER_SRC = src/server_main.cpp src/server.cpp src/expense_manager.cpp
CLI_TARGET = expense_tracker.exe
SERVER_TARGET = expense_server.exe
LIBS = -lws2_32

all: $(CLI_TARGET) $(SERVER_TARGET)

cli: $(CLI_TARGET)

server: $(SERVER_TARGET)

$(CLI_TARGET): $(CLI_SRC)
	$(CXX) $(CXXFLAGS) $(CLI_SRC) -o $(CLI_TARGET)

$(SERVER_TARGET): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) $(SERVER_SRC) $(LIBS) -o $(SERVER_TARGET)

clean:
	del $(CLI_TARGET) $(SERVER_TARGET) 2>nul || rm -f $(CLI_TARGET) $(SERVER_TARGET)
