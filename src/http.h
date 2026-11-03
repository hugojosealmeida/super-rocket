#ifndef SUPER_ROCKET_HTTP_H
#define SUPER_ROCKET_HTTP_H

#include <stddef.h>

/* Largest request head (request line plus headers) the server will read. */
#define HTTP_MAX_REQUEST 8192
#define HTTP_PATH_MAX 256
#define HTTP_QUERY_MAX 256

typedef struct {
    char path[HTTP_PATH_MAX];
    char query[HTTP_QUERY_MAX];
} HttpRequest;

#define HTTP_OK 0
#define HTTP_INCOMPLETE 1
#define HTTP_BAD_REQUEST 2
#define HTTP_METHOD_NOT_ALLOWED 3
#define HTTP_TOO_LARGE 4

/*
 * Parses a request head in buf (len bytes).  Accepts CRLF or bare LF line
 * endings.  Only GET is supported.  Returns HTTP_INCOMPLETE until the blank
 * line that ends the headers has arrived.
 */
int http_parse_request(const char *buf, size_t len, HttpRequest *req);

/* Returns 1 when path only uses characters and segments the server serves. */
int http_path_is_safe(const char *path);

/* Decodes %XX escapes and '+' into out.  Returns 0, or -1 when malformed or too long. */
int http_url_decode(const char *in, char *out, size_t size);

/*
 * Looks up key in a query string such as "d=37&x=1".  Returns 1 and the
 * decoded value when found, 0 when absent, -1 when the value is malformed or
 * does not fit in out.
 */
int http_query_get(const char *query, const char *key, char *out, size_t size);

#endif
