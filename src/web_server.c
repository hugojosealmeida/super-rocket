#define _POSIX_C_SOURCE 200809L
#include "web_server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http.h"
#include "keys.h"
#include "term.h"
#include "theme.h"
#include "web_page.h"
#include "web_term.h"

#define MAX_CLIENTS 16
#define OUT_LIMIT 32768
#define READ_TIMEOUT 5.0
#define WRITE_TIMEOUT 5.0

typedef struct {
    int used;
    int fd;
    double deadline;
    size_t in_len, out_len, out_sent;
    char in[HTTP_MAX_REQUEST];
    char out[OUT_LIMIT];
} Client;

static Client clients[MAX_CLIENTS];
static char control_token[33];
static double last_key;

static int reply(char *out, size_t size, const char *status, const char *type,
                 const char *body, size_t len)
{
    int n = snprintf(out, size,
        "HTTP/1.0 %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\n"
        "Cache-Control: no-store\r\nPragma: no-cache\r\n"
        "X-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n",
        status, type, (unsigned long)len);
    if (n < 0 || (size_t)n + len >= size)
        return -1;
    memcpy(out + n, body, len);
    out[n + len] = '\0';
    return n + (int)len;
}

static int status_reply(char *out, size_t size, const char *status)
{
    return reply(out, size, status, "text/plain", "", 0);
}

static int parse_code(const char *text, int *code)
{
    unsigned int n = 0;
    if (!*text)
        return 0;
    for (; *text; text++) {
        if (*text < '0' || *text > '9')
            return 0;
        n = n * 10 + (unsigned int)(*text - '0');
        if (n > 255)
            return 0;
    }
    *code = (int)n;
    return n > 0;
}

int web_respond(const char *head, size_t len, char *out, size_t size, int *streaming)
{
    HttpRequest req;
    char page[24000];
    int n;
    int rc = http_parse_request(head, len, &req);
    *streaming = 0; /* Reserved; refresh now uses hidden-frame polling. */
    if (rc == HTTP_INCOMPLETE)
        return 0;
    if (rc == HTTP_TOO_LARGE)
        return status_reply(out, size, "413 Request Entity Too Large");
    if (rc == HTTP_METHOD_NOT_ALLOWED)
        return status_reply(out, size, "405 Method Not Allowed");
    if (rc != HTTP_OK)
        return status_reply(out, size, "400 Bad Request");
    if (!http_path_is_safe(req.path))
        return status_reply(out, size, "404 Not Found");

    if (strcmp(req.path, "/") == 0) {
        n = web_index_document(page, sizeof(page));
    } else if (strcmp(req.path, "/play") == 0) {
        n = web_play_document(page, sizeof(page), control_token);
    } else if (strcmp(req.path, "/frame") == 0) {
        const char *chars;
        const unsigned char *colors;
        int width, height;
        unsigned long version;
        char previous[32];
        int changed;
        web_term_latest(&chars, &colors, &width, &height, &version);
        snprintf(previous, sizeof(previous), "%lu", version);
        {
            char requested[32];
            changed = http_query_get(req.query, "v", requested, sizeof(requested)) != 1 ||
                strcmp(previous, requested) != 0;
        }
        (void)colors;
        n = web_update_document(page, sizeof(page), chars, width, height, version,
                                changed, theme_color(THEME_TITLE));
    } else if (strcmp(req.path, "/key") == 0) {
        /* A random per-server capability prevents other websites sending keys. */
        char token[40];
        char value[16];
        char cleared[8];
        int code;
        int d, u, clear;
        static const char gif[] =
            "GIF89a\001\000\001\000\200\000\000\000\000\000\377\377\377"
            "\041\371\004\001\000\000\000\000\054\000\000\000\000"
            "\001\000\001\000\000\002\002\104\001\000\073";
        if (!control_token[0] || http_query_get(req.query, "t", token, sizeof(token)) != 1 ||
            strcmp(token, control_token) != 0)
            return status_reply(out, size, "403 Forbidden");
        d = http_query_get(req.query, "d", value, sizeof(value));
        u = http_query_get(req.query, "u", token, sizeof(token));
        clear = http_query_get(req.query, "clear", cleared, sizeof(cleared));
        /* Exactly one action is accepted. */
        if (d == 1 && u == 0 && clear == 0 && parse_code(value, &code)) {
            if (!keys_down(code))
                return status_reply(out, size, "400 Bad Request");
        } else if (u == 1 && d == 0 && clear == 0 && parse_code(token, &code)) {
            keys_up(code);
        } else if (clear == 1 && strcmp(cleared, "1") == 0 && d == 0 && u == 0) {
            keys_release_all();
        } else {
            return status_reply(out, size, "400 Bad Request");
        }
        last_key = term_time();
        /* A real image makes onload reliable in old browsers (unlike 204). */
        return reply(out, size, "200 OK", "image/gif", gif, sizeof(gif) - 1);
    } else {
        return status_reply(out, size, "404 Not Found");
    }
    if (n < 0)
        return -1;
    return reply(out, size, "200 OK", "text/html; charset=us-ascii", page, (size_t)n);
}

static void close_client(Client *c)
{
    if (c->used)
        close(c->fd);
    memset(c, 0, sizeof(*c));
}

static int nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    return flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

int web_server_listen(const char *addr, int port)
{
    struct sockaddr_in sa;
    unsigned char random[16];
    FILE *entropy;
    int fd, one = 1;
    size_t i;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((unsigned short)port);
    if (port < 0 || port > 65535 ||
        inet_pton(AF_INET, strcmp(addr, "localhost") == 0 ? "127.0.0.1" : addr,
                  &sa.sin_addr) != 1)
        return -1;
    entropy = fopen("/dev/urandom", "rb");
    if (entropy == NULL)
        return -1;
    i = fread(random, 1, sizeof(random), entropy);
    fclose(entropy);
    if (i != sizeof(random))
        return -1;
    for (i = 0; i < sizeof(random); i++)
        snprintf(control_token + i * 2, 3, "%02x", random[i]);
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) != 0 ||
        listen(fd, MAX_CLIENTS) != 0 || !nonblocking(fd)) {
        close(fd);
        return -1;
    }
    signal(SIGPIPE, SIG_IGN);
    last_key = term_time();
    return fd;
}

void web_server_service(int listen_fd, double timeout)
{
    struct pollfd fds[MAX_CLIENTS + 1];
    int i, ready;
    double now;
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    for (i = 0; i < MAX_CLIENTS; i++) {
        fds[i + 1].fd = clients[i].used ? clients[i].fd : -1;
        fds[i + 1].events = clients[i].out_len ? POLLOUT : POLLIN;
        fds[i + 1].revents = 0;
    }
    ready = poll(fds, MAX_CLIENTS + 1, (int)(timeout * 1000.0));
    now = term_time();
    if (now - last_key > 2.0)
        keys_release_all(); /* Lost tab, lost network, or missed key-up. */
    for (i = 0; i < MAX_CLIENTS; i++) {
        Client *c = &clients[i];
        short revents = fds[i + 1].revents;
        if (!c->used)
            continue;
        if (now >= c->deadline || (revents & (POLLERR | POLLNVAL))) {
            close_client(c);
            continue;
        }
        if (!c->out_len && (revents & (POLLIN | POLLHUP))) {
            ssize_t n = recv(c->fd, c->in + c->in_len, sizeof(c->in) - c->in_len, 0);
            if (n > 0) {
                int streaming;
                int reply_len;
                c->in_len += (size_t)n;
                reply_len = web_respond(c->in, c->in_len, c->out, sizeof(c->out), &streaming);
                if (reply_len < 0) {
                    close_client(c);
                    continue;
                }
                c->out_len = (size_t)reply_len;
                if (reply_len)
                    c->deadline = now + WRITE_TIMEOUT;
            } else if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) {
                close_client(c);
                continue;
            }
        }
        if (c->out_len && (revents & POLLOUT)) {
            ssize_t n = send(c->fd, c->out + c->out_sent, c->out_len - c->out_sent, 0);
            if (n > 0) {
                c->out_sent += (size_t)n;
                if (c->out_sent == c->out_len)
                    close_client(c);
            } else if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) {
                close_client(c);
            }
        }
    }
    if (ready > 0 && (fds[0].revents & POLLIN)) {
        /* Bound accepts per tick so connection floods cannot starve physics. */
        for (i = 0; i < MAX_CLIENTS; i++) {
            int j;
            int fd = accept(listen_fd, NULL, NULL);
            if (fd < 0)
                break;
            if (!nonblocking(fd)) {
                close(fd);
                continue;
            }
            for (j = 0; j < MAX_CLIENTS && clients[j].used; j++)
                ;
            if (j == MAX_CLIENTS) {
                static const char busy[] = "HTTP/1.0 503 Service Unavailable\r\nConnection: close\r\n\r\n";
                (void)send(fd, busy, sizeof(busy) - 1, 0);
                close(fd);
            } else {
                clients[j].used = 1;
                clients[j].fd = fd;
                clients[j].deadline = now + READ_TIMEOUT;
            }
        }
    }
}

void web_server_shutdown(int listen_fd)
{
    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
        close_client(&clients[i]);
    if (listen_fd >= 0)
        close(listen_fd);
    control_token[0] = '\0';
}
