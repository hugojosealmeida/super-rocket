#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
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

void test_term(void)
{
    shutdown_without_init_is_inert();
    init_shutdown_restores_stdin_flags();
    parent_keeps_no_nonblocking_flag();

    reset_signals();
}
