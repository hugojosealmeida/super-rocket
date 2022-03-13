CC      ?= cc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra
LDLIBS  ?= -lm
LDFLAGS ?=

BIN     := super-rocket
SRC     := src/main.c src/game.c src/render.c src/term.c src/rng.c src/util.c src/storage.c src/theme.c
HDR     := src/game.h src/render.h src/term.h src/rng.h src/util.h src/storage.h src/theme.h

TEST_BIN := tests/run_tests
TEST_SRC := tests/run_tests.c tests/test_util.c tests/test_rng.c \
            tests/test_storage.c tests/test_game.c tests/test_leaderboard.c \
            tests/test_theme.c \
            src/game.c src/rng.c src/util.c src/storage.c src/theme.c

all: $(BIN)

$(BIN): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

$(TEST_BIN): $(TEST_SRC) tests/test.h $(HDR)
	$(CC) $(CFLAGS) -Isrc -o $@ $(TEST_SRC) $(LDFLAGS) $(LDLIBS)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(BIN) $(TEST_BIN)
	rm -rf tests/run_tests.dSYM

run: $(BIN)
	./$(BIN)

.PHONY: all clean run test
