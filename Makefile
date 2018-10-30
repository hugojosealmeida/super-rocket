CC      ?= cc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra
LDLIBS  ?= -lm
LDFLAGS ?=

BIN     := super-rocket

all: $(BIN)

$(BIN): src/main.c
	$(CC) $(CFLAGS) -o $@ src/main.c $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(BIN)

run: $(BIN)
	./$(BIN)

.PHONY: all clean run
