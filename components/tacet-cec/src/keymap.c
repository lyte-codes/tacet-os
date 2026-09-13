#include "keymap.h"
#include "keycodes.h"

#include <ctype.h>
#include <errno.h>
#include <linux/input-event-codes.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

/* CEC user control codes (CEC 1.4 table 27), libcec spelling without the
 * CEC_USER_CONTROL_CODE_ prefix. Used for log messages and --test output,
 * and accepted in config as an alternative to the hex code. */
static const struct { const char *name; uint8_t code; } cec_names[] = {
    { "SELECT", 0x00 }, { "UP", 0x01 }, { "DOWN", 0x02 }, { "LEFT", 0x03 },
    { "RIGHT", 0x04 }, { "RIGHT_UP", 0x05 }, { "RIGHT_DOWN", 0x06 },
    { "LEFT_UP", 0x07 }, { "LEFT_DOWN", 0x08 }, { "ROOT_MENU", 0x09 },
    { "SETUP_MENU", 0x0A }, { "CONTENTS_MENU", 0x0B }, { "FAVORITE_MENU", 0x0C },
    { "EXIT", 0x0D }, { "TOP_MENU", 0x10 }, { "DVD_MENU", 0x11 },
    { "NUMBER_ENTRY_MODE", 0x1D }, { "NUMBER11", 0x1E }, { "NUMBER12", 0x1F },
    { "NUMBER0", 0x20 }, { "NUMBER1", 0x21 }, { "NUMBER2", 0x22 },
    { "NUMBER3", 0x23 }, { "NUMBER4", 0x24 }, { "NUMBER5", 0x25 },
    { "NUMBER6", 0x26 }, { "NUMBER7", 0x27 }, { "NUMBER8", 0x28 },
    { "NUMBER9", 0x29 }, { "DOT", 0x2A }, { "ENTER", 0x2B }, { "CLEAR", 0x2C },
    { "NEXT_FAVORITE", 0x2F }, { "CHANNEL_UP", 0x30 }, { "CHANNEL_DOWN", 0x31 },
    { "PREVIOUS_CHANNEL", 0x32 }, { "SOUND_SELECT", 0x33 }, { "INPUT_SELECT", 0x34 },
    { "DISPLAY_INFORMATION", 0x35 }, { "HELP", 0x36 }, { "PAGE_UP", 0x37 },
    { "PAGE_DOWN", 0x38 }, { "POWER", 0x40 }, { "VOLUME_UP", 0x41 },
    { "VOLUME_DOWN", 0x42 }, { "MUTE", 0x43 }, { "PLAY", 0x44 }, { "STOP", 0x45 },
    { "PAUSE", 0x46 }, { "RECORD", 0x47 }, { "REWIND", 0x48 },
    { "FAST_FORWARD", 0x49 }, { "EJECT", 0x4A }, { "FORWARD", 0x4B },
    { "BACKWARD", 0x4C }, { "STOP_RECORD", 0x4D }, { "PAUSE_RECORD", 0x4E },
    { "ANGLE", 0x50 }, { "SUB_PICTURE", 0x51 }, { "VIDEO_ON_DEMAND", 0x52 },
    { "ELECTRONIC_PROGRAM_GUIDE", 0x53 }, { "TIMER_PROGRAMMING", 0x54 },
    { "INITIAL_CONFIGURATION", 0x55 }, { "SELECT_BROADCAST_TYPE", 0x56 },
    { "SELECT_SOUND_PRESENTATION", 0x57 }, { "PLAY_FUNCTION", 0x60 },
    { "PAUSE_PLAY_FUNCTION", 0x61 }, { "RECORD_FUNCTION", 0x62 },
    { "PAUSE_RECORD_FUNCTION", 0x63 }, { "STOP_FUNCTION", 0x64 },
    { "MUTE_FUNCTION", 0x65 }, { "RESTORE_VOLUME_FUNCTION", 0x66 },
    { "TUNE_FUNCTION", 0x67 }, { "SELECT_MEDIA_FUNCTION", 0x68 },
    { "SELECT_AV_INPUT_FUNCTION", 0x69 }, { "SELECT_AUDIO_INPUT_FUNCTION", 0x6A },
    { "POWER_TOGGLE_FUNCTION", 0x6B }, { "POWER_OFF_FUNCTION", 0x6C },
    { "POWER_ON_FUNCTION", 0x6D }, { "F1_BLUE", 0x71 }, { "F2_RED", 0x72 },
    { "F3_GREEN", 0x73 }, { "F4_YELLOW", 0x74 }, { "F5", 0x75 }, { "DATA", 0x76 },
    { "AN_RETURN", 0x91 }, { "AN_CHANNELS_LIST", 0x96 },
};

/* Spec §5.1. */
static const struct { uint8_t cec; uint16_t key; bool no_repeat; } defaults[] = {
    { 0x00, KEY_ENTER, true },        /* Select */
    { 0x01, KEY_UP, false },
    { 0x02, KEY_DOWN, false },
    { 0x03, KEY_LEFT, false },
    { 0x04, KEY_RIGHT, false },
    { 0x0D, KEY_ESC, true },          /* Exit */
    { 0x09, KEY_HOME, true },         /* Root Menu */
    { 0x0A, KEY_MENU, true },         /* Setup Menu */
    { 0x0B, KEY_MENU, true },         /* Contents Menu */
    { 0x20, KEY_0, false }, { 0x21, KEY_1, false }, { 0x22, KEY_2, false },
    { 0x23, KEY_3, false }, { 0x24, KEY_4, false }, { 0x25, KEY_5, false },
    { 0x26, KEY_6, false }, { 0x27, KEY_7, false }, { 0x28, KEY_8, false },
    { 0x29, KEY_9, false },
    { 0x30, KEY_PAGEUP, false },      /* Channel Up */
    { 0x31, KEY_PAGEDOWN, false },    /* Channel Down */
    { 0x35, KEY_INFO, true },         /* Display Info */
    { 0x37, KEY_PAGEUP, false },
    { 0x38, KEY_PAGEDOWN, false },
    { 0x44, KEY_PLAY, true },
    { 0x45, KEY_STOP, true },
    { 0x46, KEY_PAUSE, true },
    { 0x48, KEY_REWIND, false },
    { 0x49, KEY_FASTFORWARD, false },
    { 0x4B, KEY_NEXTSONG, true },     /* Forward (skip) */
    { 0x4C, KEY_PREVIOUSSONG, true }, /* Backward (skip) */
    { 0x60, KEY_PLAYPAUSE, true },    /* Play/Pause */
    { 0x71, KEY_BLUE, true },
    { 0x72, KEY_RED, true },
    { 0x73, KEY_GREEN, true },
    { 0x74, KEY_YELLOW, true },
};

/* Spec §5.2. Fixed in v0.1. */
static const struct { uint8_t cec; gamepad_action act; } gamepad_table[] = {
    { 0x01, { GP_HAT_Y, -1 } },
    { 0x02, { GP_HAT_Y, +1 } },
    { 0x03, { GP_HAT_X, -1 } },
    { 0x04, { GP_HAT_X, +1 } },
    { 0x00, { GP_BUTTON, BTN_SOUTH } },   /* Select */
    { 0x0D, { GP_BUTTON, BTN_EAST } },    /* Exit */
    { 0x09, { GP_BUTTON, BTN_MODE } },    /* Root Menu */
    { 0x60, { GP_BUTTON, BTN_START } },   /* Play/Pause */
    { 0x61, { GP_BUTTON, BTN_START } },   /* Pause/Play function */
};

void keymap_defaults(keymap *m)
{
    memset(m, 0, sizeof(*m));
    for (size_t i = 0; i < ARRAY_LEN(defaults); i++) {
        m->entry[defaults[i].cec].key = defaults[i].key;
        m->entry[defaults[i].cec].no_repeat = defaults[i].no_repeat;
    }
}

const keymap_entry *keymap_lookup(const keymap *m, int cec_code)
{
    static const keymap_entry none = { KEYMAP_UNMAPPED, false };
    if (cec_code < 0 || cec_code >= KEYMAP_CEC_CODES)
        return &none;
    return &m->entry[cec_code];
}

gamepad_action keymap_gamepad(int cec_code)
{
    gamepad_action none = { GP_NONE, 0 };
    for (size_t i = 0; i < ARRAY_LEN(gamepad_table); i++)
        if (gamepad_table[i].cec == cec_code)
            return gamepad_table[i].act;
    return none;
}

static int parse_number(const char *s, long max)
{
    char *end;
    errno = 0;
    long v = strtol(s, &end, 0);
    if (errno || end == s || *end != '\0' || v < 0 || v > max)
        return -1;
    return (int)v;
}

int keymap_cec_code(const char *name)
{
    if (isdigit((unsigned char)name[0]))
        return parse_number(name, KEYMAP_CEC_CODES - 1);
    static const char prefix[] = "CEC_USER_CONTROL_CODE_";
    if (strncasecmp(name, prefix, sizeof(prefix) - 1) == 0)
        name += sizeof(prefix) - 1;
    for (size_t i = 0; i < ARRAY_LEN(cec_names); i++)
        if (strcasecmp(cec_names[i].name, name) == 0)
            return cec_names[i].code;
    return -1;
}

const char *keymap_cec_name(int cec_code)
{
    for (size_t i = 0; i < ARRAY_LEN(cec_names); i++)
        if (cec_names[i].code == cec_code)
            return cec_names[i].name;
    return NULL;
}

int keycode_from_name(const char *name)
{
    if (isdigit((unsigned char)name[0]))
        return parse_number(name, KEYCODE_MAX);
    /* keycode_names is sorted by name (generated), so binary search. */
    size_t lo = 0, hi = KEYCODE_NAME_COUNT;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        int c = strcmp(keycode_names[mid].name, name);
        if (c == 0)
            return keycode_names[mid].code;
        if (c < 0)
            lo = mid + 1;
        else
            hi = mid;
    }
    /* Second pass, case-insensitive and without the KEY_ prefix, for
     * hand-typed config ("enter", "Key_Enter"). */
    for (size_t i = 0; i < KEYCODE_NAME_COUNT; i++) {
        const char *n = keycode_names[i].name;
        if (strcasecmp(n, name) == 0 || (strncmp(n, "KEY_", 4) == 0 && strcasecmp(n + 4, name) == 0))
            return keycode_names[i].code;
    }
    return -1;
}

const char *keycode_to_name(int code)
{
    /* Prefer the canonical (first, alphabetically) name for aliased codes. */
    for (size_t i = 0; i < KEYCODE_NAME_COUNT; i++)
        if (keycode_names[i].code == code)
            return keycode_names[i].name;
    return NULL;
}

static char *trim(char *s)
{
    while (isspace((unsigned char)*s))
        s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        *--e = '\0';
    return s;
}

int keymap_apply(keymap *m, const char *lhs, const char *rhs, char *err, size_t errlen)
{
    char l[64], r[128];
    snprintf(l, sizeof(l), "%s", lhs);
    snprintf(r, sizeof(r), "%s", rhs);
    char *code_s = trim(l);
    char *val = trim(r);
    if (*code_s == '\0')
        return 0;

    int cec = keymap_cec_code(code_s);
    if (cec < 0) {
        snprintf(err, errlen, "unknown CEC code '%s'", code_s);
        return -1;
    }
    if (strcasecmp(val, "none") == 0) {
        m->entry[cec].key = KEYMAP_UNMAPPED;
        m->entry[cec].no_repeat = false;
        return 1;
    }

    bool no_repeat = m->entry[cec].no_repeat;
    char *flags = strchr(val, ',');
    if (flags) {
        *flags++ = '\0';
        for (char *tok = strtok(flags, ","); tok; tok = strtok(NULL, ",")) {
            tok = trim(tok);
            if (strcasecmp(tok, "no_repeat") == 0)
                no_repeat = true;
            else if (strcasecmp(tok, "repeat") == 0)
                no_repeat = false;
            else {
                snprintf(err, errlen, "unknown flag '%s' (expected no_repeat or repeat)", tok);
                return -1;
            }
        }
    }
    val = trim(val);
    int key = keycode_from_name(val);
    if (key < 0) {
        snprintf(err, errlen, "unknown key name '%s'", val);
        return -1;
    }
    m->entry[cec].key = (uint16_t)key;
    m->entry[cec].no_repeat = no_repeat;
    return 1;
}

size_t keymap_referenced_keys(const keymap *m, uint16_t *out, size_t max)
{
    size_t n = 0;
    for (int c = 0; c < KEYMAP_CEC_CODES; c++) {
        uint16_t k = m->entry[c].key;
        if (k == KEYMAP_UNMAPPED)
            continue;
        bool seen = false;
        for (size_t i = 0; i < n && !seen; i++)
            seen = out[i] == k;
        if (!seen && n < max)
            out[n++] = k;
    }
    return n;
}

void keymap_dump(const keymap *m, bool gamepad, FILE *f)
{
    fprintf(f, "# effective tacet-cec keymap (CEC code -> keyboard%s)\n",
            gamepad ? " / gamepad" : "");
    for (int c = 0; c < KEYMAP_CEC_CODES; c++) {
        const keymap_entry *e = &m->entry[c];
        gamepad_action g = keymap_gamepad(c);
        if (e->key == KEYMAP_UNMAPPED && (!gamepad || g.kind == GP_NONE))
            continue;
        const char *cn = keymap_cec_name(c);
        const char *kn = keycode_to_name(e->key);
        fprintf(f, "0x%02X %-24s = ", c, cn ? cn : "?");
        if (e->key == KEYMAP_UNMAPPED)
            fprintf(f, "none");
        else
            fprintf(f, "%s%s", kn ? kn : "?", e->no_repeat ? ",no_repeat" : "");
        if (gamepad && g.kind != GP_NONE) {
            if (g.kind == GP_BUTTON)
                fprintf(f, "  (gamepad %s)", keycode_to_name(g.value));
            else
                fprintf(f, "  (gamepad ABS_HAT0%c %+d)", g.kind == GP_HAT_X ? 'X' : 'Y', g.value);
        }
        fputc('\n', f);
    }
}
