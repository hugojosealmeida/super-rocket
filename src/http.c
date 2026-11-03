#include "http.h"

#include <string.h>

static int hex_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static const char *find_head_end(const char *buf, size_t len, size_t *end_len)
{
    size_t i;

    for (i = 0; i + 1 < len; i++) {
        if (buf[i] == '\n' && buf[i + 1] == '\n') {
            *end_len = 2;
            return buf + i;
        }
        if (buf[i] == '\r' && i + 3 < len && buf[i + 1] == '\n' &&
            buf[i + 2] == '\r' && buf[i + 3] == '\n') {
            *end_len = 4;
            return buf + i;
        }
    }
    return NULL;
}

int http_parse_request(const char *buf, size_t len, HttpRequest *req)
{
    size_t end_len = 0;
    const char *head_end = find_head_end(buf, len, &end_len);
    const char *line_end;
    const char *sp1;
    const char *sp2;
    const char *target;
    const char *target_end;
    const char *version;
    const char *q;
    size_t method_len;
    size_t target_len;
    size_t path_len;
    size_t query_len;
    size_t version_len;

    req->path[0] = '\0';
    req->query[0] = '\0';

    if (len > HTTP_MAX_REQUEST)
        return HTTP_TOO_LARGE;

    if (head_end == NULL)
        return len >= HTTP_MAX_REQUEST ? HTTP_TOO_LARGE : HTTP_INCOMPLETE;

    /* Reject embedded NULs and controls rather than parsing truncated strings. */
    {
        size_t i;
        size_t head_len = (size_t)(head_end - buf) + end_len;
        for (i = 0; i < head_len; i++) {
            unsigned char c = (unsigned char)buf[i];
            if ((c < 32 && c != '\r' && c != '\n' && c != '\t') || c == 127)
                return HTTP_BAD_REQUEST;
            if (c == '\r' && (i + 1 >= head_len || buf[i + 1] != '\n'))
                return HTTP_BAD_REQUEST;
        }
    }

    line_end = memchr(buf, '\n', len);
    if (line_end == NULL)
        return HTTP_BAD_REQUEST;

    sp1 = memchr(buf, ' ', (size_t)(line_end - buf));
    if (sp1 == NULL)
        return HTTP_BAD_REQUEST;

    method_len = (size_t)(sp1 - buf);
    if (method_len != 3 || memcmp(buf, "GET", 3) != 0) {
        size_t i;

        for (i = 0; i < method_len && buf[i] >= 'A' && buf[i] <= 'Z'; i++)
            ;
        if (method_len > 0 && method_len <= 16 && i == method_len)
            return HTTP_METHOD_NOT_ALLOWED;
        return HTTP_BAD_REQUEST;
    }

    target = sp1 + 1;
    sp2 = memchr(target, ' ', (size_t)(line_end - target));
    if (sp2 == NULL)
        return HTTP_BAD_REQUEST;

    version = sp2 + 1;
    version_len = (size_t)(line_end - version);
    if (version_len > 0 && version[version_len - 1] == '\r')
        version_len--;
    if (version_len != 8 || memcmp(version, "HTTP/1.", 7) != 0 ||
        (version[7] != '0' && version[7] != '1'))
        return HTTP_BAD_REQUEST;

    /* Headers are not used for routing, but malformed names/lines are refused. */
    {
        const char *p = line_end + 1;
        while (p < head_end) {
            const char *nl = memchr(p, '\n', (size_t)(head_end + end_len - p));
            const char *colon;
            const char *name;
            if (nl == NULL)
                return HTTP_BAD_REQUEST;
            colon = memchr(p, ':', (size_t)(nl - p));
            if (colon == NULL || colon == p)
                return HTTP_BAD_REQUEST;
            for (name = p; name < colon; name++) {
                unsigned char c = (unsigned char)*name;
                if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                      (c >= '0' && c <= '9') || strchr("!#$%&'*+-.^_`|~", c)))
                    return HTTP_BAD_REQUEST;
            }
            p = nl + 1;
        }
    }

    target_end = sp2;
    target_len = (size_t)(target_end - target);
    if (target_len == 0 || target[0] != '/')
        return HTTP_BAD_REQUEST;
    {
        size_t i;
        for (i = 0; i < target_len; i++)
            if ((unsigned char)target[i] <= 32 || (unsigned char)target[i] >= 127)
                return HTTP_BAD_REQUEST;
    }

    q = memchr(target, '?', target_len);
    if (q == NULL) {
        path_len = target_len;
        query_len = 0;
    } else {
        path_len = (size_t)(q - target);
        query_len = (size_t)(target_end - q - 1);
    }

    if (path_len >= HTTP_PATH_MAX || query_len >= HTTP_QUERY_MAX)
        return HTTP_TOO_LARGE;

    memcpy(req->path, target, path_len);
    req->path[path_len] = '\0';
    if (query_len > 0) {
        memcpy(req->query, q + 1, query_len);
        req->query[query_len] = '\0';
    }

    return HTTP_OK;
}

int http_path_is_safe(const char *path)
{
    const char *p;

    if (path[0] != '/')
        return 0;

    for (p = path; *p != '\0'; p++) {
        char c = *p;

        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '/' || c == '.' || c == '_' || c == '-'))
            return 0;
    }

    if (strstr(path, "..") != NULL)
        return 0;

    return 1;
}

int http_url_decode(const char *in, char *out, size_t size)
{
    size_t n = 0;

    while (*in != '\0') {
        char c = *in;

        if (c == '%') {
            int hi = hex_value(in[1]);
            int lo;

            if (in[1] == '\0' || hi < 0)
                return -1;
            lo = hex_value(in[2]);
            if (lo < 0)
                return -1;
            c = (char)(hi * 16 + lo);
            if ((unsigned char)c < 32 || (unsigned char)c == 127)
                return -1;
            in += 3;
        } else {
            if (c == '+')
                c = ' ';
            in++;
        }

        if (n + 1 >= size)
            return -1;
        out[n++] = c;
    }

    if (size == 0)
        return -1;
    out[n] = '\0';
    return 0;
}

int http_query_get(const char *query, const char *key, char *out, size_t size)
{
    size_t key_len = strlen(key);
    const char *p = query;

    while (*p != '\0') {
        const char *amp = strchr(p, '&');
        size_t pair_len = amp != NULL ? (size_t)(amp - p) : strlen(p);

        if (pair_len > key_len && p[key_len] == '=' && memcmp(p, key, key_len) == 0) {
            char raw[HTTP_QUERY_MAX];
            size_t value_len = pair_len - key_len - 1;

            if (value_len >= sizeof(raw))
                return -1;
            memcpy(raw, p + key_len + 1, value_len);
            raw[value_len] = '\0';
            return http_url_decode(raw, out, size) == 0 ? 1 : -1;
        }

        if (amp == NULL)
            break;
        p = amp + 1;
    }

    return 0;
}
