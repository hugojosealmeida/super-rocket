#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "test.h"
#include "http.h"
#include "keys.h"
#include "term.h"
#include "web_page.h"
#include "web_server.h"
#include "web_term.h"

int test_failures;
int test_checks;

static int request(const char *target, char *out, size_t size)
{
    char head[1024];
    int stream = -1;
    snprintf(head, sizeof(head), "GET %s HTTP/1.0\r\n\r\n", target);
    return web_respond(head, strlen(head), out, size, &stream);
}

int main(void)
{
    char out[32768], token[33], url[256];
    const char *chars;
    const unsigned char *colors;
    unsigned long v;
    int width, height, stream, fd;
    int stdin_flags = fcntl(STDIN_FILENO, F_GETFL);
    char *p;
    TermInput in;
    term_init();

    CHECK(web_server_listen("not-an-address", 8080) == -1);
    CHECK(web_server_listen("127.0.0.1", -1) == -1);
    fd = web_server_listen("127.0.0.1", 0);
    CHECK(fd >= 0);
    CHECK(fcntl(STDIN_FILENO, F_GETFL) == stdin_flags);
    CHECK(request("/", out, sizeof(out)) > 0);
    CHECK(strstr(out, "200 OK") != NULL);
    CHECK(strstr(out, "NAME=\"transport\"") != NULL);
    CHECK(request("/play", out, sizeof(out)) > 0);
    p = strstr(out, "var token = \"");
    CHECK(p != NULL);
    if (!p || fd < 0)
        return 1;
    memcpy(token, p + strlen("var token = \""), 32);
    token[32] = '\0';
    CHECK(request("/key?d=38", out, sizeof(out)) > 0);
    CHECK(strstr(out, "403 Forbidden") != NULL);
    snprintf(url, sizeof(url), "/key?t=%s&d=38", token);
    CHECK(request(url, out, sizeof(out)) > 0);
    CHECK(strstr(out, "200 OK") != NULL);
    CHECK(strstr(out, "image/gif") != NULL);
    term_read_input(&in);
    CHECK(in.up == 1);
    snprintf(url, sizeof(url), "/key?t=%s&u=38", token);
    CHECK(request(url, out, sizeof(out)) > 0);
    term_read_input(&in);
    CHECK(in.up == 0);
    snprintf(url, sizeof(url), "/key?t=%s&d=80", token);
    CHECK(request(url, out, sizeof(out)) > 0);
    term_read_input(&in);
    CHECK(in.pause == 1);
    term_read_input(&in);
    CHECK(in.pause == 0);
    snprintf(url, sizeof(url), "/key?t=%s&clear=1", token);
    CHECK(request(url, out, sizeof(out)) > 0);
    CHECK(strstr(out, "200 OK") != NULL);

    {
        const char *bad[] = { "d=", "d=-1", "d=256", "d=9999999999999999999999",
            "d=38x", "d=%00", "d=%zz", "clear=2", "clear=", "d=38&u=38", "d=0", "d=255" };
        size_t i;
        for (i = 0; i < sizeof(bad)/sizeof(bad[0]); i++) {
            snprintf(url, sizeof(url), "/key?t=%s&%s", token, bad[i]);
            CHECK(request(url, out, sizeof(out)) > 0);
            CHECK(strstr(out, "400 Bad Request") != NULL);
        }
    }
    CHECK(request("/../etc/passwd", out, sizeof(out)) > 0);
    CHECK(strstr(out, "404 Not Found") != NULL);
    CHECK(request("/does-not-exist", out, sizeof(out)) > 0);
    CHECK(strstr(out, "404 Not Found") != NULL);
    CHECK(web_respond("GET / HTTP/1.0\r\n", 16, out, sizeof(out), &stream) == 0);
    CHECK(request("/", out, 8) == -1);

    term_draw("ABCD", (const unsigned char *)"aaaa", 2, 2);
    web_term_latest(&chars, &colors, &width, &height, &v);
    CHECK(v == 1 && width == 2 && height == 2);
    CHECK(memcmp(chars, "ABCD", 4) == 0);
    term_draw("ABCD", (const unsigned char *)"aaaa", 2, 2);
    web_term_latest(&chars, &colors, &width, &height, &v);
    CHECK(v == 1); /* unchanged frame is not re-sent */
    term_draw("ABCD", (const unsigned char *)"bbbb", 2, 2);
    web_term_latest(&chars, &colors, &width, &height, &v);
    CHECK(v == 2); /* color changes count too */
    term_draw("x", (const unsigned char *)"x", 999, 999);
    web_term_latest(&chars, &colors, &width, &height, &v);
    CHECK(v == 2);
    CHECK(request("/frame", out, sizeof(out)) > 0);
    CHECK(strstr(out, "AB\\nCD") != NULL);
    CHECK(request("/frame?v=2", out, sizeof(out)) > 0);
    CHECK(strstr(out, "AB\\nCD") == NULL);
    CHECK(strstr(out, "setTimeout") != NULL);
    term_beep();
    CHECK(web_term_take_beep() == 1);
    CHECK(web_term_take_beep() == 0);

    CHECK(web_update_document(out, sizeof(out), "</script>\\\"\n", 12, 1, 3, 1, 97) > 0);
    CHECK(strstr(out, "\\x3c/script\\x3e") != NULL);
    CHECK(web_update_document(out, 1, "x", 1, 1, 1, 1, 97) == -1);
    CHECK(web_update_document(out, sizeof(out), "x", 81, 1, 1, 1, 97) == -1);
    CHECK(web_play_document(out, sizeof(out), "'\"</script>") == -1);
    CHECK(web_index_document(out, 8) == -1);
    web_server_shutdown(fd);
    /* Startup/shutdown must not close stdin, including the zero-initialized slots. */
    CHECK(fcntl(STDIN_FILENO, F_GETFL) == stdin_flags);
    printf("web: %d checks, %d failures\n", test_checks, test_failures);
    return test_failures != 0;
}
