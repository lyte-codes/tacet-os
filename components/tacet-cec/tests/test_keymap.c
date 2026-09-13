#include "check.h"
#include "../src/keymap.h"

#include <linux/input-event-codes.h>

void test_keymap(void)
{
    keymap m;
    keymap_defaults(&m);

    /* Spec §5.1 defaults. */
    CHECK_EQ(keymap_lookup(&m, 0x00)->key, KEY_ENTER);
    CHECK(keymap_lookup(&m, 0x00)->no_repeat);
    CHECK_EQ(keymap_lookup(&m, 0x01)->key, KEY_UP);
    CHECK(!keymap_lookup(&m, 0x01)->no_repeat);
    CHECK_EQ(keymap_lookup(&m, 0x0D)->key, KEY_ESC);
    CHECK_EQ(keymap_lookup(&m, 0x09)->key, KEY_HOME);
    CHECK_EQ(keymap_lookup(&m, 0x0A)->key, KEY_MENU);
    CHECK_EQ(keymap_lookup(&m, 0x0B)->key, KEY_MENU);
    CHECK_EQ(keymap_lookup(&m, 0x20)->key, KEY_0);
    CHECK_EQ(keymap_lookup(&m, 0x29)->key, KEY_9);
    CHECK_EQ(keymap_lookup(&m, 0x30)->key, KEY_PAGEUP);
    CHECK_EQ(keymap_lookup(&m, 0x35)->key, KEY_INFO);
    CHECK_EQ(keymap_lookup(&m, 0x44)->key, KEY_PLAY);
    CHECK_EQ(keymap_lookup(&m, 0x45)->key, KEY_STOP);
    CHECK_EQ(keymap_lookup(&m, 0x46)->key, KEY_PAUSE);
    CHECK_EQ(keymap_lookup(&m, 0x48)->key, KEY_REWIND);
    CHECK(!keymap_lookup(&m, 0x48)->no_repeat);
    CHECK_EQ(keymap_lookup(&m, 0x4B)->key, KEY_NEXTSONG);
    CHECK_EQ(keymap_lookup(&m, 0x60)->key, KEY_PLAYPAUSE);
    CHECK_EQ(keymap_lookup(&m, 0x71)->key, KEY_BLUE);
    CHECK_EQ(keymap_lookup(&m, 0x72)->key, KEY_RED);
    CHECK_EQ(keymap_lookup(&m, 0x73)->key, KEY_GREEN);
    CHECK_EQ(keymap_lookup(&m, 0x74)->key, KEY_YELLOW);
    /* Volume is deliberately unmapped by default. */
    CHECK_EQ(keymap_lookup(&m, 0x41)->key, KEYMAP_UNMAPPED);
    CHECK_EQ(keymap_lookup(&m, 0x42)->key, KEYMAP_UNMAPPED);
    CHECK_EQ(keymap_lookup(&m, 0x43)->key, KEYMAP_UNMAPPED);
    /* Out of range is unmapped, not a crash. */
    CHECK_EQ(keymap_lookup(&m, -1)->key, KEYMAP_UNMAPPED);
    CHECK_EQ(keymap_lookup(&m, 999)->key, KEYMAP_UNMAPPED);

    /* Spec §5.2 gamepad table. */
    CHECK_EQ(keymap_gamepad(0x01).kind, GP_HAT_Y); CHECK_EQ(keymap_gamepad(0x01).value, -1);
    CHECK_EQ(keymap_gamepad(0x02).kind, GP_HAT_Y); CHECK_EQ(keymap_gamepad(0x02).value, +1);
    CHECK_EQ(keymap_gamepad(0x03).kind, GP_HAT_X); CHECK_EQ(keymap_gamepad(0x03).value, -1);
    CHECK_EQ(keymap_gamepad(0x04).kind, GP_HAT_X); CHECK_EQ(keymap_gamepad(0x04).value, +1);
    CHECK_EQ(keymap_gamepad(0x00).kind, GP_BUTTON); CHECK_EQ(keymap_gamepad(0x00).value, BTN_SOUTH);
    CHECK_EQ(keymap_gamepad(0x0D).value, BTN_EAST);
    CHECK_EQ(keymap_gamepad(0x09).value, BTN_MODE);
    CHECK_EQ(keymap_gamepad(0x60).value, BTN_START);
    CHECK_EQ(keymap_gamepad(0x41).kind, GP_NONE);

    /* Names. */
    CHECK_EQ(keymap_cec_code("SELECT"), 0x00);
    CHECK_EQ(keymap_cec_code("select"), 0x00);
    CHECK_EQ(keymap_cec_code("CEC_USER_CONTROL_CODE_VOLUME_UP"), 0x41);
    CHECK_EQ(keymap_cec_code("0x41"), 0x41);
    CHECK_EQ(keymap_cec_code("65"), 0x41);
    CHECK_EQ(keymap_cec_code("0x100"), -1);
    CHECK_EQ(keymap_cec_code("bogus"), -1);
    CHECK_STR(keymap_cec_name(0x0D), "EXIT");
    CHECK(keymap_cec_name(0x99) == NULL);

    /* Generated keycode table. */
    CHECK_EQ(keycode_from_name("KEY_ENTER"), KEY_ENTER);
    CHECK_EQ(keycode_from_name("BTN_SOUTH"), BTN_SOUTH);
    CHECK_EQ(keycode_from_name("BTN_GAMEPAD"), BTN_SOUTH);   /* alias resolved */
    CHECK_EQ(keycode_from_name("key_enter"), KEY_ENTER);
    CHECK_EQ(keycode_from_name("enter"), KEY_ENTER);
    CHECK_EQ(keycode_from_name("28"), KEY_ENTER);
    CHECK_EQ(keycode_from_name("0x1c"), KEY_ENTER);
    CHECK_EQ(keycode_from_name("KEY_NOPE"), -1);
    CHECK_EQ(keycode_from_name("99999"), -1);
    CHECK_STR(keycode_to_name(KEY_ENTER), "KEY_ENTER");
    CHECK_STR(keycode_to_name(KEY_INFO), "KEY_INFO");
    CHECK(keycode_to_name(0x7FF) == NULL);

    /* Config merge (spec §6 [keymap]). */
    char err[128] = "";
    CHECK_EQ(keymap_apply(&m, "0x41", "KEY_VOLUMEUP", err, sizeof(err)), 1);
    CHECK_EQ(keymap_lookup(&m, 0x41)->key, KEY_VOLUMEUP);
    CHECK(!keymap_lookup(&m, 0x41)->no_repeat);
    CHECK_EQ(keymap_apply(&m, "0x43", "KEY_MUTE,no_repeat", err, sizeof(err)), 1);
    CHECK_EQ(keymap_lookup(&m, 0x43)->key, KEY_MUTE);
    CHECK(keymap_lookup(&m, 0x43)->no_repeat);
    CHECK_EQ(keymap_apply(&m, "0x43", "KEY_MUTE , no_repeat", err, sizeof(err)), 1);
    CHECK_EQ(keymap_apply(&m, "0x01", "KEY_UP,repeat", err, sizeof(err)), 1);
    CHECK(!keymap_lookup(&m, 0x01)->no_repeat);
    CHECK_EQ(keymap_apply(&m, "0x71", "none", err, sizeof(err)), 1);
    CHECK_EQ(keymap_lookup(&m, 0x71)->key, KEYMAP_UNMAPPED);
    CHECK_EQ(keymap_apply(&m, "VOLUME_DOWN", "KEY_VOLUMEDOWN", err, sizeof(err)), 1);
    CHECK_EQ(keymap_lookup(&m, 0x42)->key, KEY_VOLUMEDOWN);
    /* Overriding a no_repeat default keeps the flag unless told otherwise. */
    CHECK_EQ(keymap_apply(&m, "0x00", "KEY_SPACE", err, sizeof(err)), 1);
    CHECK(keymap_lookup(&m, 0x00)->no_repeat);
    /* Errors leave the map alone. */
    CHECK_EQ(keymap_apply(&m, "0x00", "KEY_BOGUS", err, sizeof(err)), -1);
    CHECK_EQ(keymap_lookup(&m, 0x00)->key, KEY_SPACE);
    CHECK(strstr(err, "KEY_BOGUS") != NULL);
    CHECK_EQ(keymap_apply(&m, "0x1FF", "KEY_A", err, sizeof(err)), -1);
    CHECK_EQ(keymap_apply(&m, "0x00", "KEY_A,bogus_flag", err, sizeof(err)), -1);
    CHECK_EQ(keymap_apply(&m, "", "KEY_A", err, sizeof(err)), 0);

    /* Referenced keys: unique, no zero. */
    keymap d;
    keymap_defaults(&d);
    uint16_t keys[KEYMAP_CEC_CODES];
    size_t n = keymap_referenced_keys(&d, keys, KEYMAP_CEC_CODES);
    CHECK(n >= 25 && n < 40);
    bool has_enter = false, dup = false;
    for (size_t i = 0; i < n; i++) {
        CHECK(keys[i] != 0);
        if (keys[i] == KEY_ENTER) has_enter = true;
        for (size_t j = i + 1; j < n; j++) if (keys[i] == keys[j]) dup = true;
    }
    CHECK(has_enter);
    CHECK(!dup);
}
