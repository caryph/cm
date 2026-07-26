CXX     := c++
CFLAGS  := -Wall -Wextra -std=c++17 -O2 $(shell pkg-config --cflags raylib)
LDFLAGS := $(shell pkg-config --libs raylib)

SRC     := src/main.cpp
BIN     := game

$(BIN): $(SRC)
	$(CXX) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)

.PHONY: run clean
