#include <string.h>

#include "test.h"
#include "http.h"

static int parse(const char *text, HttpRequest *req)
{
    return http_parse_request(text, strlen(text), req);
}

void test_http(void)
{
    HttpRequest req;
    char out[64];
    char big[HTTP_MAX_REQUEST + 16];
    char long_path[HTTP_PATH_MAX + 16];

    /* happy path */
    CHECK(parse("GET / HTTP/1.0\r\n\r\n", &req) == HTTP_OK);
    CHECK(strcmp(req.path, "/") == 0);
    CHECK(req.query[0] == '\0');

    CHECK(parse("GET /key?d=37 HTTP/1.1\r\nHost: localhost\r\n\r\n", &req) == HTTP_OK);
    CHECK(strcmp(req.path, "/key") == 0);
    CHECK(strcmp(req.query, "d=37") == 0);

    /* bare LF line endings are accepted */
    CHECK(parse("GET /screen HTTP/1.0\n\n", &req) == HTTP_OK);
    CHECK(strcmp(req.path, "/screen") == 0);

    /* edge: incomplete head, waiting for the blank line */
    CHECK(parse("GET / HTTP/1.0\r\nHost: x\r\n", &req) == HTTP_INCOMPLETE);
    CHECK(parse("", &req) == HTTP_INCOMPLETE);

    /* edge: methods other than GET */
    CHECK(parse("HEAD / HTTP/1.0\r\n\r\n", &req) == HTTP_METHOD_NOT_ALLOWED);
    CHECK(parse("POST /key HTTP/1.0\r\n\r\n", &req) == HTTP_METHOD_NOT_ALLOWED);

    /* edge: malformed request lines */
    CHECK(parse("GET/ HTTP/1.0\r\n\r\n", &req) == HTTP_BAD_REQUEST);
    CHECK(parse("GET HTTP/1.0\r\n\r\n", &req) == HTTP_BAD_REQUEST);
    CHECK(parse("GET index.html HTTP/1.0\r\n\r\n", &req) == HTTP_BAD_REQUEST);
    CHECK(parse("GET / HTTP/2.0\r\n\r\n", &req) == HTTP_BAD_REQUEST);
    CHECK(parse("GET / FTP/1.0\r\n\r\n", &req) == HTTP_BAD_REQUEST);
    CHECK(parse("GET / \r\n\r\n", &req) == HTTP_BAD_REQUEST);
    CHECK(parse("get / HTTP/1.0\r\n\r\n", &req) == HTTP_BAD_REQUEST);
    CHECK(parse("GET / HTTP/1.0\r\nBroken header\r\n\r\n", &req) == HTTP_BAD_REQUEST);
    CHECK(parse("GET / HTTP/1.0\r\nBad name: x\r\n\r\n", &req) == HTTP_BAD_REQUEST);
    CHECK(parse("GET / HTTP/1.0\r\n: x\r\n\r\n", &req) == HTTP_BAD_REQUEST);
    CHECK(parse("GET / HTTP/1.0\r\nHost: local\r\nX-Test: yes\r\n\r\n", &req) == HTTP_OK);
    CHECK(parse("GET /a\tb HTTP/1.0\r\n\r\n", &req) == HTTP_BAD_REQUEST);
    {
        const char nul[] = "GET /\0evil HTTP/1.0\r\n\r\n";
        CHECK(http_parse_request(nul, sizeof(nul) - 1, &req) == HTTP_BAD_REQUEST);
    }

    /* edge: a head that fills the buffer without ending */
    memset(big, 'a', sizeof(big) - 1);
    big[sizeof(big) - 1] = '\0';
    CHECK(http_parse_request(big, HTTP_MAX_REQUEST, &req) == HTTP_TOO_LARGE);
    CHECK(http_parse_request(big, HTTP_MAX_REQUEST - 1, &req) == HTTP_INCOMPLETE);

    /* edge: path and query length limits */
    memset(long_path, 'p', sizeof(long_path) - 1);
    long_path[0] = '/';
    long_path[sizeof(long_path) - 1] = '\0';
    snprintf(big, sizeof(big), "GET %s HTTP/1.0\r\n\r\n", long_path);
    CHECK(parse(big, &req) == HTTP_TOO_LARGE);

    snprintf(big, sizeof(big), "GET /key?%s HTTP/1.0\r\n\r\n", long_path);
    CHECK(parse(big, &req) == HTTP_TOO_LARGE);

    /* path safety: only plain paths the server serves */
    CHECK(http_path_is_safe("/"));
    CHECK(http_path_is_safe("/key"));
    CHECK(http_path_is_safe("/static/app-1.0_x.js"));
    CHECK(!http_path_is_safe("/../etc/passwd"));
    CHECK(!http_path_is_safe("/a/../b"));
    CHECK(!http_path_is_safe("/a%2e%2e/b"));
    CHECK(!http_path_is_safe("/a\\b"));
    CHECK(!http_path_is_safe("/a b"));
    CHECK(!http_path_is_safe("relative"));
    CHECK(!http_path_is_safe(""));

    /* url decoding */
    CHECK(http_url_decode("abc", out, sizeof(out)) == 0);
    CHECK(strcmp(out, "abc") == 0);
    CHECK(http_url_decode("a+b%20c%2F", out, sizeof(out)) == 0);
    CHECK(strcmp(out, "a b c/") == 0);
    CHECK(http_url_decode("", out, sizeof(out)) == 0);
    CHECK(out[0] == '\0');
    CHECK(http_url_decode("%zz", out, sizeof(out)) == -1);
    CHECK(http_url_decode("100%", out, sizeof(out)) == -1);
    CHECK(http_url_decode("%2", out, sizeof(out)) == -1);
    CHECK(http_url_decode("%00", out, sizeof(out)) == -1);
    CHECK(http_url_decode("%0d%0a", out, sizeof(out)) == -1);
    CHECK(http_url_decode("abcdef", out, 4) == -1);
    CHECK(http_url_decode("abc", out, 0) == -1);

    /* query lookup */
    CHECK(http_query_get("d=37&u=0", "d", out, sizeof(out)) == 1);
    CHECK(strcmp(out, "37") == 0);
    CHECK(http_query_get("d=37&u=40", "u", out, sizeof(out)) == 1);
    CHECK(strcmp(out, "40") == 0);
    CHECK(http_query_get("d=37", "x", out, sizeof(out)) == 0);
    CHECK(http_query_get("", "d", out, sizeof(out)) == 0);
    CHECK(http_query_get("dd=1", "d", out, sizeof(out)) == 0);
    CHECK(http_query_get("xd=1&d=2", "d", out, sizeof(out)) == 1);
    CHECK(strcmp(out, "2") == 0);
    CHECK(http_query_get("d=", "d", out, sizeof(out)) == 1);
    CHECK(out[0] == '\0');
    CHECK(http_query_get("d", "d", out, sizeof(out)) == 0);
    CHECK(http_query_get("d=%zz", "d", out, sizeof(out)) == -1);
    CHECK(http_query_get("d=12345", "d", out, 4) == -1);

    /* Every truncated prefix remains safe to parse, even without a NUL terminator. */
    {
        const char *complete = "GET /key?d=37 HTTP/1.0\r\nHost: local\r\n\r\n";
        size_t i;
        for (i = 0; i < strlen(complete); i++)
            CHECK(http_parse_request(complete, i, &req) == HTTP_INCOMPLETE);
        CHECK(http_parse_request(complete, strlen(complete), &req) == HTTP_OK);
    }
}
