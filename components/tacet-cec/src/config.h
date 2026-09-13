/* /etc/tacet/cec.conf (INI) -> config struct (spec §6). Pure. */
#ifndef TACET_CONFIG_H
#define TACET_CONFIG_H

#include "keymap.h"
#include "log.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CONFIG_DEVICE_NAME_MAX 12   /* CEC OSD name; libcec 6 stores 13 bytes with terminator */
#define CONFIG_PATH_MAX        256
#define CONFIG_DEFAULT_PATH    "/etc/tacet/cec.conf"
#define CONFIG_FALLBACK_PATH   "/usr/share/tacet/cec.conf.default"

typedef enum {
    LA_PLAYBACK1 = 4, LA_PLAYBACK2 = 8, LA_PLAYBACK3 = 11, LA_RECORDING1 = 1
} logical_address_pref;

typedef struct config {
    /* [cec] */
    char     adapter[CONFIG_PATH_MAX];   /* "auto" or a path */
    logical_address_pref logical_address;
    uint16_t physical_address;           /* 0xFFFF = auto, else a.b.c.d packed as nibbles */
    char     device_name[CONFIG_DEVICE_NAME_MAX + 1];
    bool     reclaim_active_source;
    /* [input] */
    bool     gamepad;
    bool     gamepad_mirror;
    int      repeat_delay_ms;
    int      repeat_rate_ms;
    int      repeat_timeout_ms;
    int      debounce_ms;
    /* [keymap] */
    keymap   map;
    /* [log] */
    log_level level;
} config;

void config_defaults(config *c);

/* Applies one section/key/value. Returns 1 on change, 0 if ignored (unknown
 * key; err explains), -1 if the value was rejected (err explains, config
 * unchanged). */
int  config_apply(config *c, const char *section, const char *key, const char *value,
                  char *err, size_t errlen);

/* Parses an INI file. Bad lines are logged with their number and skipped
 * (spec §9); returns the number of bad lines, or -1 if unreadable. */
int  config_load_file(config *c, const char *path);

/* Loads CONFIG_DEFAULT_PATH, else CONFIG_FALLBACK_PATH, else built-ins.
 * `explicit_path`, when non-NULL, is the only file tried. Returns 0, or -1 if
 * an explicit path could not be read. */
int  config_load(config *c, const char *explicit_path);

/* Helpers exposed for tests. */
int  config_parse_physical_address(const char *s, uint16_t *out);  /* "1.0.0.0" -> 0x1000 */
int  config_parse_bool(const char *s, bool *out);

#endif
