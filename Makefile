CC      ?= cc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra
LDLIBS  ?= -lm
LDFLAGS ?=

BIN     := super-rocket
SRC     := src/main.c src/game.c src/render.c src/term.c
HDR     := src/game.h src/render.h src/term.h

all: $(BIN)

$(BIN): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(BIN)

run: $(BIN)
	./$(BIN)

.PHONY: all clean run
