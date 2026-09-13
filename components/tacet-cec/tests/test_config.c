#include "check.h"
#include "../src/config.h"

#include <linux/input-event-codes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const char *write_temp(const char *content)
{
    static char path[] = "/tmp/tacet-cec-test-XXXXXX";
    static char copy[64];
    snprintf(copy, sizeof(copy), "%s", path);
    int fd = mkstemp(copy);
    FILE *f = fdopen(fd, "w");
    fputs(content, f);
    fclose(f);
    return copy;
}

void test_config(void)
{
    config c;
    config_defaults(&c);
    CHECK_STR(c.adapter, "auto");
    CHECK_EQ(c.logical_address, LA_PLAYBACK1);
    CHECK_EQ(c.physical_address, 0xFFFF);
    CHECK_STR(c.device_name, "Tacet");
    CHECK(c.reclaim_active_source);
    CHECK(c.gamepad);
    CHECK(c.gamepad_mirror);
    CHECK_EQ(c.repeat_delay_ms, 400);
    CHECK_EQ(c.repeat_rate_ms, 100);
    CHECK_EQ(c.repeat_timeout_ms, 2000);
    CHECK_EQ(c.debounce_ms, 50);
    CHECK_EQ(c.level, LOG_INFO);
    CHECK_EQ(keymap_lookup(&c.map, 0x00)->key, KEY_ENTER);

    uint16_t pa;
    CHECK_EQ(config_parse_physical_address("1.0.0.0", &pa), 0); CHECK_EQ(pa, 0x1000);
    CHECK_EQ(config_parse_physical_address("2.1.0.0", &pa), 0); CHECK_EQ(pa, 0x2100);
    CHECK_EQ(config_parse_physical_address("auto", &pa), 0);    CHECK_EQ(pa, 0xFFFF);
    CHECK_EQ(config_parse_physical_address("16.0.0.0", &pa), -1);
    CHECK_EQ(config_parse_physical_address("1.0.0", &pa), -1);
    CHECK_EQ(config_parse_physical_address("1.0.0.0x", &pa), -1);

    bool b;
    CHECK_EQ(config_parse_bool("true", &b), 0);  CHECK(b);
    CHECK_EQ(config_parse_bool("OFF", &b), 0);   CHECK(!b);
    CHECK_EQ(config_parse_bool("maybe", &b), -1);

    /* Spec §6 example, plus comments, spacing and mistakes. */
    const char *path = write_temp(
        "; leading comment\n"
        "[cec]\n"
        "adapter = /dev/cec0            ; inline comment\n"
        "logical_address = recording1\n"
        "physical_address = 1.0.0.0\n"
        "device_name = Living Room\n"
        "reclaim_active_source = false\n"
        "\n"
        "[input]\n"
        "gamepad = false\n"
        "gamepad_mirror = no\n"
        "repeat_delay_ms = 300\n"
        "repeat_rate_ms = 5      # too small, keeps default\n"
        "repeat_timeout_ms = 1500\n"
        "debounce_ms = abc\n"
        "unknown_key = 1\n"
        "\n"
        "[keymap]\n"
        "0x41 = KEY_VOLUMEUP\n"
        "0x42 = KEY_VOLUMEDOWN\n"
        "0x43 = KEY_MUTE,no_repeat\n"
        "0x71 = none\n"
        "0x00 = KEY_NOPE\n"
        "\n"
        "[log]\n"
        "level = debug\n"
        "\n"
        "[extra]\n"
        "whatever = 1\n"
        "garbage line without equals\n");
    config_defaults(&c);
    int bad = config_load_file(&c, path);
    unlink(path);
    CHECK_EQ(bad, 4);   /* repeat_rate_ms, debounce_ms, KEY_NOPE, garbage line */
    CHECK_STR(c.adapter, "/dev/cec0");
    CHECK_EQ(c.logical_address, LA_RECORDING1);
    CHECK_EQ(c.physical_address, 0x1000);
    CHECK_STR(c.device_name, "Living Room");
    CHECK(!c.reclaim_active_source);
    CHECK(!c.gamepad);
    CHECK(!c.gamepad_mirror);
    CHECK_EQ(c.repeat_delay_ms, 300);
    CHECK_EQ(c.repeat_rate_ms, 100);
    CHECK_EQ(c.repeat_timeout_ms, 1500);
    CHECK_EQ(c.debounce_ms, 50);
    CHECK_EQ(keymap_lookup(&c.map, 0x41)->key, KEY_VOLUMEUP);
    CHECK_EQ(keymap_lookup(&c.map, 0x43)->key, KEY_MUTE);
    CHECK(keymap_lookup(&c.map, 0x43)->no_repeat);
    CHECK_EQ(keymap_lookup(&c.map, 0x71)->key, KEYMAP_UNMAPPED);
    CHECK_EQ(keymap_lookup(&c.map, 0x00)->key, KEY_ENTER);   /* bad line kept default */
    CHECK_EQ(c.level, LOG_DEBUG);

    /* Individual rejections. */
    char err[128];
    config_defaults(&c);
    CHECK_EQ(config_apply(&c, "cec", "device_name", "ThisNameIsWayTooLongForCEC", err, sizeof(err)), -1);
    CHECK_STR(c.device_name, "Tacet");
    CHECK_EQ(config_apply(&c, "cec", "logical_address", "tv", err, sizeof(err)), -1);
    CHECK_EQ(config_apply(&c, "cec", "logical_address", "PLAYBACK3", err, sizeof(err)), 1);
    CHECK_EQ(c.logical_address, LA_PLAYBACK3);
    CHECK_EQ(config_apply(&c, "input", "repeat_delay_ms", "10", err, sizeof(err)), -1);
    CHECK_EQ(config_apply(&c, "input", "repeat_delay_ms", "50", err, sizeof(err)), 1);
    CHECK_EQ(config_apply(&c, "log", "level", "loud", err, sizeof(err)), -1);
    CHECK_EQ(config_apply(&c, "log", "level", "warn", err, sizeof(err)), 1);
    CHECK_EQ(config_apply(&c, "nope", "x", "1", err, sizeof(err)), 0);
    CHECK_EQ(config_apply(&c, "cec", "x", "1", err, sizeof(err)), 0);

    /* Missing file. */
    CHECK_EQ(config_load_file(&c, "/nonexistent/cec.conf"), -1);
    CHECK_EQ(config_load(&c, "/nonexistent/cec.conf"), -1);
}
