/*
 * super-rocket
 *
 * A small terminal game about flying a rocket from Earth to Mars.
 */

#include <stdio.h>

static const char *ROCKET[] = {
    "      /\\      ",
    "     /  \\     ",
    "    /    \\    ",
    "   |      |   ",
    "   |      |   ",
    "   |      |   ",
    "  /|      |\\  ",
    " / |      | \\ ",
    "/__|______|__\\",
    "   /|    |\\   ",
    "  / |    | \\  ",
    " /  |    |  \\ ",
    "/___|____|___\\"
};

int main(void)
{
    printf("\033[2J\033[H");

    printf("\n\n");
    printf("        S U P E R   R O C K E T\n");
    printf("      Earth ---------------------> Mars\n\n\n");

    for (unsigned i = 0; i < sizeof(ROCKET) / sizeof(ROCKET[0]); i++) {
        printf("                    %s\n", ROCKET[i]);
    }

    printf("\n\n");
    printf("      The year is 2018. Humanity is going to Mars.\n");
    printf("      You have one rocket, one pilot and a lot of space.\n\n");
    printf("                  Press ENTER to continue...\n");

    fflush(stdout);
    getchar();

    printf("\033[2J\033[H");
    printf("Launch pad is still under construction.\n");

    return 0;
}
