CC      ?= cc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra
LDLIBS  ?= -lm
LDFLAGS ?=
EMCC    ?= emcc

BIN     := super-rocket
SRC     := src/main.c src/app.c src/game.c src/render.c src/term.c src/rng.c src/util.c src/storage.c src/theme.c
HDR     := src/app.h src/game.h src/render.h src/term.h src/rng.h src/util.h src/storage.h src/theme.h
WEB_BIN := super-rocket-web
WEB_SRC := src/web_main.c src/web_server.c src/web_term.c src/web_clock.c src/app.c src/game.c src/render.c src/rng.c src/util.c src/storage.c src/theme.c src/http.c src/keys.c src/web_page.c
WEB_HDR := $(HDR) src/http.h src/keys.h src/web_page.h src/web_term.h src/web_server.h src/web_clock.h
WEB_TEST_BIN := tests/run_web_tests
WEB_TEST_SRC := tests/run_web_tests.c $(filter-out src/web_main.c,$(WEB_SRC))
SANFLAGS := -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer

TEST_BIN := tests/run_tests
TEST_SRC := tests/run_tests.c tests/test_util.c tests/test_rng.c \
            tests/test_storage.c tests/test_game.c tests/test_settings.c \
            tests/test_leaderboard.c tests/test_theme.c \
            tests/test_campaign.c tests/test_combat.c tests/test_term.c \
            tests/test_version.c tests/test_http.c tests/test_keys.c tests/test_web_page.c tests/test_app.c \
            src/game.c src/rng.c src/util.c src/storage.c src/theme.c src/term.c \
            src/http.c src/keys.c src/web_page.c src/app.c src/render.c

CHANGELOG := CHANGELOG.md

all: $(BIN)

web: $(WEB_BIN)

wasm:
	python3 tools/build_wasm.py --emcc "$(EMCC)"

test-wasm-build:
	python3 tests/test_wasm_build.py

$(WEB_BIN): $(WEB_SRC) $(WEB_HDR)
	$(CC) $(CFLAGS) -o $@ $(WEB_SRC) $(LDFLAGS) $(LDLIBS)

$(WEB_TEST_BIN): $(WEB_TEST_SRC) $(WEB_HDR)
	$(CC) $(CFLAGS) -Isrc -o $@ $(WEB_TEST_SRC) $(LDFLAGS) $(LDLIBS)

test-web: $(WEB_TEST_BIN) $(WEB_BIN)
	./$(WEB_TEST_BIN)
	python3 tests/test_web_integration.py ./$(WEB_BIN)

tests/run_tests_san: $(TEST_SRC) tests/test.h $(WEB_HDR)
	$(CC) $(SANFLAGS) -Isrc -o $@ $(TEST_SRC) $(LDLIBS)

tests/run_web_tests_san: $(WEB_TEST_SRC) tests/test.h $(WEB_HDR)
	$(CC) $(SANFLAGS) -Isrc -o $@ $(WEB_TEST_SRC) $(LDLIBS)

tests/super-rocket-web-san: $(WEB_SRC) $(WEB_HDR)
	$(CC) $(SANFLAGS) -o $@ $(WEB_SRC) $(LDLIBS)

test-sanitize: tests/run_tests_san tests/run_web_tests_san tests/super-rocket-web-san
	./tests/run_tests_san
	./tests/run_web_tests_san
	python3 tests/test_web_integration.py ./tests/super-rocket-web-san

$(BIN): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

$(TEST_BIN): $(TEST_SRC) tests/test.h $(WEB_HDR)
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
	rm -f $(BIN) $(TEST_BIN) $(WEB_BIN) $(WEB_TEST_BIN)
	rm -f tests/run_tests_san tests/run_web_tests_san tests/super-rocket-web-san
	rm -rf tests/run_tests.dSYM
	rm -f dist/wasm/rocket.js dist/wasm/rocket.wasm dist/wasm/index.html dist/wasm/client.js

run: $(BIN)
	./$(BIN)

.PHONY: all web wasm clean run test test-web test-wasm-build test-sanitize check-version
