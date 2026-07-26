CC      := cc
CFLAGS  := -Wall -Wextra -std=c11 -O2 $(shell pkg-config --cflags raylib)
LDFLAGS := $(shell pkg-config --libs raylib)

SRC     := src/main.c
BIN     := game

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)

.PHONY: run clean
