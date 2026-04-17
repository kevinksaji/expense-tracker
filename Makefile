CXX      := clang++
CXXFLAGS := -std=c++23 -Wall -Wextra -Iinclude
LDFLAGS  := -lsqlite3

SRCS := src/main.cpp src/csv_parser.cpp src/classifier.cpp src/db.cpp src/query.cpp
BIN  := expense-tracker

.PHONY: all clean

all: $(BIN)

$(BIN): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) -o $(BIN)

clean:
	rm -f $(BIN) data/transactions.db
