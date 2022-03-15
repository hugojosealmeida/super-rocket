#ifndef SUPER_ROCKET_TEST_H
#define SUPER_ROCKET_TEST_H

#include <stdio.h>

extern int test_failures;
extern int test_checks;

void test_report(const char *name, int ok);

#define CHECK(cond)                                                           \
    do {                                                                      \
        test_checks++;                                                        \
        if (!(cond)) {                                                        \
            test_failures++;                                                  \
            fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                     \
    } while (0)

void test_util(void);
void test_rng(void);
void test_storage(void);
void test_game(void);
void test_leaderboard(void);
void test_theme(void);
void test_campaign(void);
void test_combat(void);

#endif
