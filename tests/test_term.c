#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#if !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1
#endif

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "term.h"
#include "test.h"

/*
 * Issue #3: term_init() puts O_NONBLOCK on stdin, but that is a status flag of
 * the open file description the shell handed us, so it outlives the game unless
 * term_shutdown() puts the original flags back.
 */

static int flags_of(int fd)
{
    return fcntl(fd, F_GETFL);
}

/* term_init()/term_shutdown() write escape sequences: keep them out of the log. */
static int mute_stdout(void)
{
    int devnull;
    int saved;

    fflush(stdout);

    devnull = open("/dev/null", O_RDWR);
    if (devnull < 0)
        return -1;

    saved = dup(STDOUT_FILENO);
    if (saved < 0 || dup2(devnull, STDOUT_FILENO) < 0) {
        if (saved >= 0)
            close(saved);
        close(devnull);
        return -1;
    }

    close(devnull);
    return saved;
}

static void unmute_stdout(int saved)
{
    if (saved < 0)
        return;

    fflush(stdout);
    dup2(saved, STDOUT_FILENO);
    close(saved);
}

/* term_init() installs handlers; the test process should keep default ones. */
static void reset_signals(void)
{
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
}

/* Shutdown before any init must not touch stdin, no matter how often it runs. */
static void shutdown_without_init_is_inert(void)
{
    int before = flags_of(STDIN_FILENO);

    term_shutdown();
    term_shutdown();

    CHECK(flags_of(STDIN_FILENO) == before);
}

/* One game session hands stdin back to the shell with its original flags. */
static void init_shutdown_restores_stdin_flags(void)
{
    int before = flags_of(STDIN_FILENO);
    int during;
    int after;
    int muted;

    if (before == -1) {
        CHECK(0); /* no usable stdin to test with */
        return;
    }

    muted = mute_stdout();
    term_init();
    during = flags_of(STDIN_FILENO);
    term_shutdown();
    after = flags_of(STDIN_FILENO);
    unmute_stdout(muted);
    reset_signals();

    /* in-game input still reads non-blocking ... */
    CHECK((during & O_NONBLOCK) != 0);
    /* ... and the flag is gone afterwards.  A tty reports extra kernel-managed
       bits through F_GETFL, so compare the flag that we actually set */
    CHECK((after & O_NONBLOCK) == (before & O_NONBLOCK));

    /* a second session behaves the same way */
    muted = mute_stdout();
    term_init();
    during = flags_of(STDIN_FILENO);
    term_shutdown();
    after = flags_of(STDIN_FILENO);
    unmute_stdout(muted);
    reset_signals();

    CHECK((during & O_NONBLOCK) != 0);
    CHECK((after & O_NONBLOCK) == (before & O_NONBLOCK));
}

/*
 * The leak is only visible from the side that shares the open file description,
 * so this mirrors the shell: the child dup2()s the read end of a pipe onto
 * stdin (the very same description the parent keeps), plays and quits, and the
 * parent then inspects its own descriptor.
 */
static void parent_keeps_no_nonblocking_flag(void)
{
    int fds[2];
    int original;
    int now;
    int status;
    pid_t child;

    if (pipe(fds) != 0) {
        CHECK(0);
        return;
    }

    /* the parent (the "shell") starts from a plain blocking descriptor */
    original = flags_of(fds[0]);
    CHECK(original != -1);
    CHECK((original & O_NONBLOCK) == 0);

    child = fork();
    if (child < 0) {
        CHECK(0);
        close(fds[0]);
        close(fds[1]);
        return;
    }

    if (child == 0) {
        TermInput in;
        int failed = 0;
        int during;
        int after;

        /* if the non-blocking read ever stalls, die loudly instead of hanging */
        alarm(10);

        if (dup2(fds[0], STDIN_FILENO) < 0)
            _exit(3);
        close(fds[0]);
        /* the write end stays open in the parent, so stdin can never reach EOF */
        close(fds[1]);

        mute_stdout();

        term_init();
        during = flags_of(STDIN_FILENO);

        /* no data yet and no EOF: this returns only when stdin is non-blocking */
        term_read_input(&in);

        term_shutdown();
        after = flags_of(STDIN_FILENO);

        if ((during & O_NONBLOCK) == 0)
            failed = 1;
        if (after != original)
            failed = 2;

        alarm(0);
        _exit(failed);
    }

    if (waitpid(child, &status, 0) < 0)
        status = -1;

    now = flags_of(fds[0]);
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    CHECK(now == original);
    CHECK((now & O_NONBLOCK) == 0);

    close(fds[0]);
    close(fds[1]);
}

/*
 * Issue #4: a terminal smaller than 40x12 used to be silently "handled" by
 * drawing an 80-column frame into it: every line wrapped, cursor addressing
 * landed on the wrong rows and the game became unplayable.  term_size() now
 * answers TERM_SIZE_TOO_SMALL with the actual size, while a stdout without a
 * tty (pipes, CI) keeps the historical 80x23 fallback.
 *
 * The size comes from ioctl(TIOCGWINSZ) on stdout, so each case forks a
 * child whose stdout is the fixture under test - a pty of a chosen size, or
 * a pipe - and reports pass/fail through its exit status.
 */

#define SIZE_SETUP_FAILED 20 /* child could not install its stdout fixture */

static int make_pty_pair(int cols, int rows, int *master, int *slave)
{
    struct winsize ws;
    char *name;

    *master = posix_openpt(O_RDWR | O_NOCTTY);
    if (*master < 0)
        return -1;

    if (grantpt(*master) != 0 || unlockpt(*master) != 0) {
        close(*master);
        *master = -1;
        return -1;
    }

    name = ptsname(*master);
    if (name == NULL) {
        close(*master);
        *master = -1;
        return -1;
    }

    *slave = open(name, O_RDWR | O_NOCTTY);
    if (*slave < 0) {
        close(*master);
        *master = -1;
        return -1;
    }

    memset(&ws, 0, sizeof(ws));
    ws.ws_col = (unsigned short)cols;
    ws.ws_row = (unsigned short)rows;
    if (ioctl(*slave, TIOCSWINSZ, &ws) != 0 && ioctl(*master, TIOCSWINSZ, &ws) != 0) {
        close(*slave);
        close(*master);
        *slave = -1;
        *master = -1;
        return -1;
    }

    return 0;
}

/* cols < 0 asks for a pipe on stdout (a size-less, tty-less descriptor). */
static void check_size_case(const char *label, int cols, int rows,
                            int want_rc, int want_w, int want_h)
{
    int master = -1;
    int slave = -1;
    int fds[2] = { -1, -1 };
    int status = -1;
    pid_t child;

    if (cols >= 0) {
        if (make_pty_pair(cols, rows, &master, &slave) != 0) {
            CHECK(0); /* this environment offers no pty to test with */
            return;
        }
    } else if (pipe(fds) != 0) {
        CHECK(0);
        return;
    }

    child = fork();
    if (child < 0) {
        CHECK(0);
    } else if (child == 0) {
        int rc;
        int w = -1;
        int h = -1;

        /* never let a stuck fixture hang the suite */
        alarm(10);

        if (cols >= 0) {
            if (dup2(slave, STDOUT_FILENO) < 0)
                _exit(SIZE_SETUP_FAILED);
        } else {
            close(fds[0]);
            if (dup2(fds[1], STDOUT_FILENO) < 0)
                _exit(SIZE_SETUP_FAILED);
        }

        rc = term_size(&w, &h);

        if (rc != want_rc || w != want_w || h != want_h)
            fprintf(stderr, "  term_size %s: got rc=%d %dx%d, want rc=%d %dx%d\n",
                    label, rc, w, h, want_rc, want_w, want_h);

        alarm(0);
        _exit(rc != want_rc || w != want_w || h != want_h);
    }

    if (waitpid(child, &status, 0) < 0)
        status = -1;

    if (master >= 0)
        close(master);
    if (slave >= 0)
        close(slave);
    if (fds[0] >= 0) {
        close(fds[0]);
        close(fds[1]);
    }

    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

static void size_refuses_terminals_that_are_too_small(void)
{
    check_size_case("30x10", 30, 10, TERM_SIZE_TOO_SMALL, 30, 10);
    /* one row / one column below the minimum on each axis */
    check_size_case("39x12", 39, 12, TERM_SIZE_TOO_SMALL, 39, 12);
    check_size_case("40x11", 40, 11, TERM_SIZE_TOO_SMALL, 40, 11);
}

static void size_accepts_the_smallest_playable_terminal(void)
{
    check_size_case("40x12", 40, 12, TERM_SIZE_OK, 40, 11);
    check_size_case("80x24", 80, 24, TERM_SIZE_OK, 80, 23);
}

static void size_falls_back_when_stdout_is_no_tty(void)
{
    check_size_case("pipe stdout", -1, 0, TERM_SIZE_OK, 80, 23);
}

void test_term(void)
{
    shutdown_without_init_is_inert();
    size_refuses_terminals_that_are_too_small();
    size_accepts_the_smallest_playable_terminal();
    size_falls_back_when_stdout_is_no_tty();
    init_shutdown_restores_stdin_flags();
    parent_keeps_no_nonblocking_flag();

    reset_signals();
}
