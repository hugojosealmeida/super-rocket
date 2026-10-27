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

# ASan/UBSan build of the same unit suite (make test-san). The flags are
# deliberately fixed instead of reusing CFLAGS so the sanitizer net cannot
# be configured away: -g keeps reports mapped to source lines, -O1 keeps
# builds fast, -fno-sanitize-recover makes every UBSan report fatal (exit
# non-zero) instead of just printing it, -fno-omit-frame-pointer keeps the
# stack traces readable. Requires a compiler with ASan/UBSan support (gcc
# or clang); CI runs it on ubuntu-latest. Opt-in locally, mandatory in CI.
TEST_BIN_SAN := tests/run_tests_san
SAN_CFLAGS   := -std=c99 -g -O1 -Wall -Wextra \
                -fsanitize=address,undefined -fno-sanitize-recover=all \
                -fno-omit-frame-pointer

CHANGELOG := CHANGELOG.md

all: $(BIN)

$(BIN): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

$(TEST_BIN): $(TEST_SRC) tests/test.h $(HDR)
	$(CC) $(CFLAGS) -Isrc -o $@ $(TEST_SRC) $(LDFLAGS) $(LDLIBS)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN_SAN): $(TEST_SRC) tests/test.h $(HDR)
	$(CC) $(SAN_CFLAGS) -Isrc -o $@ $(TEST_SRC) $(LDFLAGS) $(LDLIBS)

test-san: $(TEST_BIN_SAN)
	UBSAN_OPTIONS=print_stacktrace=1 ./$(TEST_BIN_SAN)

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
	rm -f $(BIN) $(TEST_BIN) $(TEST_BIN_SAN)
	rm -rf tests/run_tests.dSYM tests/run_tests_san.dSYM

run: $(BIN)
	./$(BIN)

.PHONY: all clean run test test-san check-version
