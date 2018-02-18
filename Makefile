CC      ?= cc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra
LDFLAGS ?=

BIN     := super-rocket

all: $(BIN)

$(BIN): src/main.c
	$(CC) $(CFLAGS) -o $@ src/main.c $(LDFLAGS)

clean:
	rm -f $(BIN)

run: $(BIN)
	./$(BIN)

.PHONY: all clean run
