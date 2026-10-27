CC      ?= cc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra
LDLIBS  ?= -lm
LDFLAGS ?=

BIN     := super-rocket
SRC     := src/main.c src/game.c src/render.c src/term.c src/rng.c src/util.c src/storage.c src/theme.c
HDR     := src/game.h src/render.h src/term.h src/rng.h src/util.h src/storage.h src/theme.h

TEST_BIN := tests/run_tests
TEST_SRC := tests/run_tests.c tests/test_util.c tests/test_rng.c \
            tests/test_storage.c tests/test_game.c tests/test_settings.c \
            tests/test_leaderboard.c tests/test_theme.c \
            tests/test_campaign.c tests/test_combat.c tests/test_term.c \
            tests/test_version.c \
            src/game.c src/rng.c src/util.c src/storage.c src/theme.c src/term.c

CHANGELOG := CHANGELOG.md

all: $(BIN)

$(BIN): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

$(TEST_BIN): $(TEST_SRC) tests/test.h $(HDR)
	$(CC) $(CFLAGS) -Isrc -o $@ $(TEST_SRC) $(LDFLAGS) $(LDLIBS)

test: $(TEST_BIN)
	./$(TEST_BIN)

# Assert that the version `./super-rocket --version` prints (GAME_VERSION,
# also used by the menu footer) matches the newest released `## <version>`
# heading in CHANGELOG.md, so the two can never silently drift apart.
# `## Unreleased` and any other non-numeric heading are skipped; the first
# `## <digit>...` line wins and its trailing date is dropped.
check-version: $(BIN)
	@documented=`sed -n 's/^## \([0-9][^[:space:]]*\).*/\1/p' $(CHANGELOG) | head -n 1`; \
	printed=`./$(BIN) --version | sed 's/^super-rocket //'`; \
	if [ -z "$$documented" ]; then \
		echo "check-version: no released version heading found in $(CHANGELOG)" >&2; \
		exit 1; \
	fi; \
	if [ "$$printed" != "$$documented" ]; then \
		echo "check-version: --version prints '$$printed' but $(CHANGELOG) documents '$$documented'" >&2; \
		exit 1; \
	fi; \
	echo "check-version: super-rocket $$printed matches $(CHANGELOG)"

clean:
	rm -f $(BIN) $(TEST_BIN)
	rm -rf tests/run_tests.dSYM

run: $(BIN)
	./$(BIN)

.PHONY: all clean run test check-version
