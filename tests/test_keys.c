#include <string.h>

#include "test.h"
#include "keys.h"

void test_keys(void)
{
    TermInput in;

    keys_reset();

    /* happy path: a held arrow keeps thrusting until released */
    CHECK(keys_down(37) == 1);
    keys_take(&in);
    CHECK(in.left == 1);
    CHECK(in.right == 0);
    keys_take(&in);
    CHECK(in.left == 1);
    keys_up(37);
    keys_take(&in);
    CHECK(in.left == 0);

    /* WASD and arrows map to the same thrust directions */
    keys_down('W');
    keys_take(&in);
    CHECK(in.up == 1);
    keys_up('W');
    keys_down(40);
    keys_take(&in);
    CHECK(in.down == 1);
    keys_up(40);
    keys_down('D');
    keys_take(&in);
    CHECK(in.right == 1);
    keys_up('D');
    keys_take(&in);
    CHECK(in.right == 0);

    /* Space fires while held */
    keys_down(32);
    keys_take(&in);
    CHECK(in.fire == 1);
    keys_up(32);
    keys_take(&in);
    CHECK(in.fire == 0);

    /* one-shot keys fire once per press, not on autorepeat */
    keys_down('P');
    keys_take(&in);
    CHECK(in.pause == 1);
    keys_down('P');
    keys_take(&in);
    CHECK(in.pause == 0);
    keys_up('P');
    keys_down('P');
    keys_take(&in);
    CHECK(in.pause == 1);
    keys_up('P');
    keys_take(&in);

    /* edge: a one-shot is consumed by the take that reads it */
    keys_down('T');
    keys_take(&in);
    CHECK(in.theme == 1);
    keys_take(&in);
    CHECK(in.theme == 0);
    keys_up('T');

    /* letters set the letter field for initials entry, and the action */
    keys_down('X');
    keys_take(&in);
    CHECK(in.letter == 'X');
    keys_up('X');

    /* digits select difficulty and upgrades; other fields stay clear */
    keys_down('2');
    keys_take(&in);
    CHECK(in.digit == 2);
    CHECK(in.letter == -1);
    CHECK(in.pause == 0);
    keys_up('2');
    keys_take(&in);
    CHECK(in.digit == -1);

    /* Enter, Esc (quit) and Backspace */
    keys_down(13);
    keys_take(&in);
    CHECK(in.enter == 1);
    keys_up(13);
    keys_down(27);
    keys_take(&in);
    CHECK(in.quit == 1);
    keys_up(27);
    keys_down(8);
    keys_take(&in);
    CHECK(in.backspace == 1);
    keys_up(8);

    /* edge: unknown and out-of-range codes are ignored */
    CHECK(keys_down(0) == 0);
    CHECK(keys_down(123) == 0);
    CHECK(keys_down(-1) == 0);
    CHECK(keys_down(300) == 0);
    keys_take(&in);
    CHECK(in.up == 0 && in.fire == 0 && in.quit == 0 && in.letter == -1);

    /* edge: keyup for a key that was never pressed is harmless */
    keys_up(38);
    keys_up(-5);
    keys_up(999);
    keys_take(&in);
    CHECK(in.up == 0);

    /* edge: window blur releases held keys but keeps queued one-shots */
    keys_down(38);
    keys_down(32);
    keys_release_all();
    keys_take(&in);
    CHECK(in.up == 0);
    CHECK(in.fire == 0);

    keys_down('H');
    keys_release_all();
    keys_take(&in);
    CHECK(in.help == 1);

    /* edge: the same key can be pressed again after a blur */
    keys_down(38);
    keys_take(&in);
    CHECK(in.up == 1);
    keys_up(38);

    keys_reset();
    keys_take(&in);
    CHECK(in.up == 0 && in.help == 0 && in.digit == -1);
}
