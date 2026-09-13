#include "keymap.h"

#include <ctype.h>
#include <errno.h>
#include <linux/input-event-codes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef struct { const char *name; uint16_t code; } name_entry;

/* CEC user control codes (CEC 1.4 table 27, libcec cectypes.h). */
static const name_entry cec_names[] = {
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

/* Linux keys the bridge knows by name. The first block (ESC..D, codes 1..31)
 * is complete on purpose: udev's input_id only tags a device
 * ID_INPUT_KEYBOARD when all of those are present, and libinput/gamescope
 * treat untagged devices as not-a-keyboard. */
static const name_entry key_names[] = {
    { "KEY_ESC", KEY_ESC }, { "KEY_1", KEY_1 }, { "KEY_2", KEY_2 }, { "KEY_3", KEY_3 },
    { "KEY_4", KEY_4 }, { "KEY_5", KEY_5 }, { "KEY_6", KEY_6 }, { "KEY_7", KEY_7 },
    { "KEY_8", KEY_8 }, { "KEY_9", KEY_9 }, { "KEY_0", KEY_0 }, { "KEY_MINUS", KEY_MINUS },
    { "KEY_EQUAL", KEY_EQUAL }, { "KEY_BACKSPACE", KEY_BACKSPACE }, { "KEY_TAB", KEY_TAB },
    { "KEY_Q", KEY_Q }, { "KEY_W", KEY_W }, { "KEY_E", KEY_E }, { "KEY_R", KEY_R },
    { "KEY_T", KEY_T }, { "KEY_Y", KEY_Y }, { "KEY_U", KEY_U }, { "KEY_I", KEY_I },
    { "KEY_O", KEY_O }, { "KEY_P", KEY_P }, { "KEY_LEFTBRACE", KEY_LEFTBRACE },
    { "KEY_RIGHTBRACE", KEY_RIGHTBRACE }, { "KEY_ENTER", KEY_ENTER },
    { "KEY_LEFTCTRL", KEY_LEFTCTRL }, { "KEY_A", KEY_A }, { "KEY_S", KEY_S },
    { "KEY_D", KEY_D }, { "KEY_F", KEY_F }, { "KEY_G", KEY_G }, { "KEY_H", KEY_H },
    { "KEY_J", KEY_J }, { "KEY_K", KEY_K }, { "KEY_L", KEY_L },
    { "KEY_SEMICOLON", KEY_SEMICOLON }, { "KEY_APOSTROPHE", KEY_APOSTROPHE },
    { "KEY_GRAVE", KEY_GRAVE }, { "KEY_LEFTSHIFT", KEY_LEFTSHIFT },
    { "KEY_BACKSLASH", KEY_BACKSLASH }, { "KEY_Z", KEY_Z }, { "KEY_X", KEY_X },
    { "KEY_C", KEY_C }, { "KEY_V", KEY_V }, { "KEY_B", KEY_B }, { "KEY_N", KEY_N },
    { "KEY_M", KEY_M }, { "KEY_COMMA", KEY_COMMA }, { "KEY_DOT", KEY_DOT },
    { "KEY_SLASH", KEY_SLASH }, { "KEY_RIGHTSHIFT", KEY_RIGHTSHIFT },
    { "KEY_LEFTALT", KEY_LEFTALT }, { "KEY_SPACE", KEY_SPACE },
    { "KEY_CAPSLOCK", KEY_CAPSLOCK },
    { "KEY_F1", KEY_F1 }, { "KEY_F2", KEY_F2 }, { "KEY_F3", KEY_F3 }, { "KEY_F4", KEY_F4 },
    { "KEY_F5", KEY_F5 }, { "KEY_F6", KEY_F6 }, { "KEY_F7", KEY_F7 }, { "KEY_F8", KEY_F8 },
    { "KEY_F9", KEY_F9 }, { "KEY_F10", KEY_F10 }, { "KEY_F11", KEY_F11 }, { "KEY_F12", KEY_F12 },
    { "KEY_HOME", KEY_HOME }, { "KEY_UP", KEY_UP }, { "KEY_PAGEUP", KEY_PAGEUP },
    { "KEY_LEFT", KEY_LEFT }, { "KEY_RIGHT", KEY_RIGHT }, { "KEY_END", KEY_END },
    { "KEY_DOWN", KEY_DOWN }, { "KEY_PAGEDOWN", KEY_PAGEDOWN }, { "KEY_INSERT", KEY_INSERT },
    { "KEY_DELETE", KEY_DELETE }, { "KEY_MUTE", KEY_MUTE }, { "KEY_VOLUMEDOWN", KEY_VOLUMEDOWN },
    { "KEY_VOLUMEUP", KEY_VOLUMEUP }, { "KEY_POWER", KEY_POWER }, { "KEY_PAUSE", KEY_PAUSE },
    { "KEY_COMPOSE", KEY_COMPOSE }, { "KEY_MENU", KEY_MENU }, { "KEY_STOP", KEY_STOP },
    { "KEY_SLEEP", KEY_SLEEP }, { "KEY_BACK", KEY_BACK }, { "KEY_FORWARD", KEY_FORWARD },
    { "KEY_EJECTCD", KEY_EJECTCD }, { "KEY_NEXTSONG", KEY_NEXTSONG },
    { "KEY_PLAYPAUSE", KEY_PLAYPAUSE }, { "KEY_PREVIOUSSONG", KEY_PREVIOUSSONG },
    { "KEY_STOPCD", KEY_STOPCD }, { "KEY_RECORD", KEY_RECORD }, { "KEY_REWIND", KEY_REWIND },
    { "KEY_HOMEPAGE", KEY_HOMEPAGE }, { "KEY_EXIT", KEY_EXIT }, { "KEY_PLAY", KEY_PLAY },
    { "KEY_FASTFORWARD", KEY_FASTFORWARD }, { "KEY_MEDIA", KEY_MEDIA },
    /* Above the X11 range; accepted so a Wayland-native shell can use them,
     * and flagged by keymap_key_fits_x11(). */
    { "KEY_OK", KEY_OK }, { "KEY_SELECT", KEY_SELECT }, { "KEY_INFO", KEY_INFO },
    { "KEY_SUBTITLE", KEY_SUBTITLE }, { "KEY_RED", KEY_RED }, { "KEY_GREEN", KEY_GREEN },
    { "KEY_YELLOW", KEY_YELLOW }, { "KEY_BLUE", KEY_BLUE }, { "KEY_CHANNELUP", KEY_CHANNELUP },
    { "KEY_CHANNELDOWN", KEY_CHANNELDOWN }, { "KEY_EPG", KEY_EPG },
    { "KEY_CONTEXT_MENU", KEY_CONTEXT_MENU }, { "KEY_NEXT", KEY_NEXT },
    { "KEY_PREVIOUS", KEY_PREVIOUS },
};

/* Default mapping. Every target is a key Kodi's stock keyboard.xml binds and
 * that fits the X11 keycode range. SPEC.md §3 explains each choice. */
static const struct { uint8_t cec; uint16_t key; } defaults[] = {
    { 0x00, KEY_ENTER },        /* SELECT */
    { 0x2B, KEY_ENTER },        /* ENTER */
    { 0x01, KEY_UP }, { 0x02, KEY_DOWN }, { 0x03, KEY_LEFT }, { 0x04, KEY_RIGHT },
    { 0x09, KEY_HOMEPAGE },     /* ROOT_MENU  -> Kodi home */
    { 0x10, KEY_HOMEPAGE },     /* TOP_MENU */
    { 0x0A, KEY_COMPOSE },      /* SETUP_MENU    -> Kodi context menu */
    { 0x0B, KEY_COMPOSE },      /* CONTENTS_MENU */
    { 0x0C, KEY_COMPOSE },      /* FAVORITE_MENU */
    { 0x11, KEY_COMPOSE },      /* DVD_MENU */
    { 0x0D, KEY_BACKSPACE },    /* EXIT      -> Kodi back */
    { 0x91, KEY_BACKSPACE },    /* AN_RETURN (Samsung "return") */
    { 0x20, KEY_0 }, { 0x21, KEY_1 }, { 0x22, KEY_2 }, { 0x23, KEY_3 }, { 0x24, KEY_4 },
    { 0x25, KEY_5 }, { 0x26, KEY_6 }, { 0x27, KEY_7 }, { 0x28, KEY_8 }, { 0x29, KEY_9 },
    { 0x2A, KEY_DOT },
    { 0x2C, KEY_DELETE },       /* CLEAR */
    { 0x30, KEY_PAGEUP },       /* CHANNEL_UP */
    { 0x31, KEY_PAGEDOWN },     /* CHANNEL_DOWN */
    { 0x37, KEY_PAGEUP }, { 0x38, KEY_PAGEDOWN },
    { 0x35, KEY_I },            /* DISPLAY_INFORMATION -> Kodi info */
    { 0x53, KEY_E },            /* ELECTRONIC_PROGRAM_GUIDE -> Kodi TV guide */
    { 0x51, KEY_T },            /* SUB_PICTURE -> Kodi subtitles toggle */
    { 0x41, KEY_VOLUMEUP }, { 0x42, KEY_VOLUMEDOWN }, { 0x43, KEY_MUTE },
    { 0x65, KEY_MUTE },
    { 0x44, KEY_PLAYPAUSE }, { 0x46, KEY_PLAYPAUSE },
    { 0x60, KEY_PLAYPAUSE }, { 0x61, KEY_PLAYPAUSE },
    { 0x45, KEY_STOPCD }, { 0x64, KEY_STOPCD },
    { 0x47, KEY_RECORD }, { 0x62, KEY_RECORD },
    { 0x48, KEY_REWIND },
    { 0x49, KEY_FASTFORWARD },
    { 0x4A, KEY_EJECTCD },
    { 0x4B, KEY_NEXTSONG },     /* FORWARD  -> skip next */
    { 0x4C, KEY_PREVIOUSSONG }, /* BACKWARD -> skip previous */
    { 0x72, KEY_F1 },           /* F2_RED */
    { 0x73, KEY_F2 },           /* F3_GREEN */
    { 0x74, KEY_F3 },           /* F4_YELLOW */
    { 0x71, KEY_F4 },           /* F1_BLUE */
};

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

void keymap_defaults(keymap *m)
{
    memset(m, 0, sizeof(*m));
    for (size_t i = 0; i < ARRAY_LEN(defaults); i++)
        m->key[defaults[i].cec] = defaults[i].key;
}

uint16_t keymap_lookup(const keymap *m, int cec_code)
{
    if (cec_code < 0 || cec_code >= KEYMAP_CEC_CODES)
        return KEYMAP_UNMAPPED;
    return m->key[cec_code];
}

static int parse_number(const char *s, int max)
{
    char *end;
    errno = 0;
    long v = strtol(s, &end, 0);
    if (errno || end == s || *end != '\0' || v < 0 || v > max)
        return -1;
    return (int)v;
}

static int lookup_name(const name_entry *table, size_t n, const char *name,
                       const char *prefix)
{
    if (isdigit((unsigned char)name[0]))
        return -2; /* caller handles numbers */
    size_t plen = strlen(prefix);
    if (strncasecmp(name, prefix, plen) == 0)
        name += plen;
    for (size_t i = 0; i < n; i++) {
        const char *tn = table[i].name;
        if (strncasecmp(tn, prefix, plen) == 0)
            tn += plen;
        if (strcasecmp(tn, name) == 0)
            return table[i].code;
    }
    return -1;
}

int keymap_cec_code(const char *name)
{
    int r = lookup_name(cec_names, ARRAY_LEN(cec_names), name, "CEC_USER_CONTROL_CODE_");
    if (r == -2)
        return parse_number(name, KEYMAP_CEC_CODES - 1);
    return r;
}

const char *keymap_cec_name(int cec_code)
{
    for (size_t i = 0; i < ARRAY_LEN(cec_names); i++)
        if (cec_names[i].code == cec_code)
            return cec_names[i].name;
    return NULL;
}

int keymap_key_code(const char *name)
{
    int r = lookup_name(key_names, ARRAY_LEN(key_names), name, "KEY_");
    if (r == -2)
        return parse_number(name, KEY_MAX);
    return r;
}

const char *keymap_key_name(int key_code)
{
    for (size_t i = 0; i < ARRAY_LEN(key_names); i++)
        if (key_names[i].code == key_code)
            return key_names[i].name;
    return NULL;
}

bool keymap_key_fits_x11(int key_code)
{
    return key_code > 0 && key_code <= KEYMAP_X11_MAX_KEY;
}

size_t keymap_known_key_count(void)
{
    return ARRAY_LEN(key_names);
}

uint16_t keymap_known_key(size_t index)
{
    return index < ARRAY_LEN(key_names) ? key_names[index].code : 0;
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

int keymap_parse_line(keymap *m, const char *line, char *err, size_t errlen)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", line);
    char *hash = strchr(buf, '#');
    if (hash)
        *hash = '\0';
    char *s = trim(buf);
    if (*s == '\0')
        return 0;

    /* "CEC KEY", "CEC=KEY" or "CEC = KEY" */
    char *sep = strpbrk(s, " \t=");
    if (!sep) {
        snprintf(err, errlen, "expected 'CEC_CODE KEY_NAME', got '%s'", s);
        return -1;
    }
    *sep = '\0';
    char *rhs = sep + 1;
    while (*rhs == ' ' || *rhs == '\t' || *rhs == '=')
        rhs++;
    rhs = trim(rhs);
    if (*rhs == '\0' || strpbrk(rhs, " \t")) {
        snprintf(err, errlen, "expected exactly one key after '%s'", s);
        return -1;
    }

    int cec = keymap_cec_code(s);
    if (cec < 0) {
        snprintf(err, errlen, "unknown CEC code '%s'", s);
        return -1;
    }
    int key;
    if (strcasecmp(rhs, "none") == 0) {
        key = KEYMAP_UNMAPPED;
    } else {
        key = keymap_key_code(rhs);
        if (key < 0) {
            snprintf(err, errlen, "unknown key '%s'", rhs);
            return -1;
        }
    }
    m->key[cec] = (uint16_t)key;
    return 1;
}

int keymap_load_file(keymap *m, const char *path, char *err, size_t errlen)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        snprintf(err, errlen, "%s: %s", path, strerror(errno));
        return -1;
    }
    char line[256];
    char lerr[192];
    int bad = 0, lineno = 0;
    while (fgets(line, sizeof(line), f)) {
        lineno++;
        if (keymap_parse_line(m, line, lerr, sizeof(lerr)) < 0) {
            bad++;
            snprintf(err, errlen, "%s:%d: %s", path, lineno, lerr);
        }
    }
    fclose(f);
    return bad;
}
