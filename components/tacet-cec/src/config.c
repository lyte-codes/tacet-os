#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

void config_defaults(config *c)
{
    memset(c, 0, sizeof(*c));
    snprintf(c->adapter, sizeof(c->adapter), "auto");
    c->logical_address = LA_PLAYBACK1;
    c->physical_address = 0xFFFF;
    snprintf(c->device_name, sizeof(c->device_name), "Tacet");
    c->reclaim_active_source = true;
    c->gamepad = true;
    c->gamepad_mirror = true;
    c->repeat_delay_ms = 400;
    c->repeat_rate_ms = 100;
    c->repeat_timeout_ms = 2000;
    c->debounce_ms = 50;
    keymap_defaults(&c->map);
    c->level = LOG_INFO;
}

int config_parse_bool(const char *s, bool *out)
{
    static const char *const yes[] = { "true", "yes", "on", "1" };
    static const char *const no[]  = { "false", "no", "off", "0" };
    for (size_t i = 0; i < 4; i++) {
        if (strcasecmp(s, yes[i]) == 0) { *out = true;  return 0; }
        if (strcasecmp(s, no[i])  == 0) { *out = false; return 0; }
    }
    return -1;
}

int config_parse_physical_address(const char *s, uint16_t *out)
{
    if (strcasecmp(s, "auto") == 0) {
        *out = 0xFFFF;
        return 0;
    }
    unsigned a, b, c, d;
    char tail;
    if (sscanf(s, "%u.%u.%u.%u%c", &a, &b, &c, &d, &tail) != 4 || a > 15 || b > 15 || c > 15 || d > 15)
        return -1;
    *out = (uint16_t)((a << 12) | (b << 8) | (c << 4) | d);
    return 0;
}

static int parse_ms(const char *v, int lo, int hi, int *out)
{
    char *end;
    errno = 0;
    long n = strtol(v, &end, 10);
    if (errno || end == v || *end || n < lo || n > hi)
        return -1;
    *out = (int)n;
    return 0;
}

int config_apply(config *c, const char *section, const char *key, const char *value,
                 char *err, size_t errlen)
{
    bool b;
    if (strcasecmp(section, "cec") == 0) {
        if (strcasecmp(key, "adapter") == 0) {
            if (*value == '\0' || strlen(value) >= CONFIG_PATH_MAX) {
                snprintf(err, errlen, "adapter must be auto or a device path");
                return -1;
            }
            snprintf(c->adapter, sizeof(c->adapter), "%s", value);
            return 1;
        }
        if (strcasecmp(key, "logical_address") == 0) {
            static const struct { const char *n; logical_address_pref la; } las[] = {
                { "playback1", LA_PLAYBACK1 }, { "playback2", LA_PLAYBACK2 },
                { "playback3", LA_PLAYBACK3 }, { "recording1", LA_RECORDING1 },
            };
            for (size_t i = 0; i < 4; i++)
                if (strcasecmp(value, las[i].n) == 0) { c->logical_address = las[i].la; return 1; }
            snprintf(err, errlen, "logical_address must be playback1|playback2|playback3|recording1");
            return -1;
        }
        if (strcasecmp(key, "physical_address") == 0) {
            uint16_t pa;
            if (config_parse_physical_address(value, &pa) < 0) {
                snprintf(err, errlen, "physical_address must be auto or a.b.c.d (each 0-15)");
                return -1;
            }
            c->physical_address = pa;
            return 1;
        }
        if (strcasecmp(key, "device_name") == 0) {
            size_t n = strlen(value);
            if (n == 0 || n > CONFIG_DEVICE_NAME_MAX) {
                snprintf(err, errlen, "device_name must be 1..%d characters", CONFIG_DEVICE_NAME_MAX);
                return -1;
            }
            snprintf(c->device_name, sizeof(c->device_name), "%s", value);
            return 1;
        }
        if (strcasecmp(key, "reclaim_active_source") == 0) {
            if (config_parse_bool(value, &b) < 0) { snprintf(err, errlen, "%s must be true or false", key); return -1; }
            c->reclaim_active_source = b;
            return 1;
        }
    } else if (strcasecmp(section, "input") == 0) {
        if (strcasecmp(key, "gamepad") == 0 || strcasecmp(key, "gamepad_mirror") == 0) {
            if (config_parse_bool(value, &b) < 0) { snprintf(err, errlen, "%s must be true or false", key); return -1; }
            if (strcasecmp(key, "gamepad") == 0) c->gamepad = b; else c->gamepad_mirror = b;
            return 1;
        }
        struct { const char *n; int *dst; int lo, hi; } ms[] = {
            { "repeat_delay_ms",   &c->repeat_delay_ms,   50, 5000 },
            { "repeat_rate_ms",    &c->repeat_rate_ms,    20, 2000 },
            { "repeat_timeout_ms", &c->repeat_timeout_ms, 100, 60000 },
            { "debounce_ms",       &c->debounce_ms,       0, 1000 },
        };
        for (size_t i = 0; i < 4; i++) {
            if (strcasecmp(key, ms[i].n) != 0)
                continue;
            int v;
            if (parse_ms(value, ms[i].lo, ms[i].hi, &v) < 0) {
                snprintf(err, errlen, "%s must be an integer %d..%d", key, ms[i].lo, ms[i].hi);
                return -1;
            }
            *ms[i].dst = v;
            return 1;
        }
    } else if (strcasecmp(section, "keymap") == 0) {
        return keymap_apply(&c->map, key, value, err, errlen);
    } else if (strcasecmp(section, "log") == 0) {
        if (strcasecmp(key, "level") == 0) {
            int l = log_level_from_name(value);
            if (l < 0) { snprintf(err, errlen, "level must be error|warn|info|debug"); return -1; }
            c->level = (log_level)l;
            return 1;
        }
    } else {
        snprintf(err, errlen, "ignoring unknown section [%s]", section);
        return 0;
    }
    snprintf(err, errlen, "ignoring unknown key '%s' in [%s]", key, section);
    return 0;
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

/* Strips ; and # comments, but not inside a value that starts a quoted
 * string — v0.1 has no quoted values, so a comment starts at the first ; or
 * # preceded by start-of-line or whitespace. */
static void strip_comment(char *s)
{
    for (char *p = s; *p; p++) {
        if ((*p == ';' || *p == '#') && (p == s || isspace((unsigned char)p[-1]))) {
            *p = '\0';
            return;
        }
    }
}

int config_load_file(config *c, const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        log_warn("%s: %s", path, strerror(errno));
        return -1;
    }
    char line[512], section[64] = "", err[256];
    int bad = 0, lineno = 0;
    while (fgets(line, sizeof(line), f)) {
        lineno++;
        strip_comment(line);
        char *s = trim(line);
        if (*s == '\0')
            continue;
        if (*s == '[') {
            char *close = strchr(s, ']');
            if (!close) {
                log_warn("%s:%d: unterminated section header", path, lineno);
                bad++;
                continue;
            }
            *close = '\0';
            snprintf(section, sizeof(section), "%s", trim(s + 1));
            continue;
        }
        char *eq = strchr(s, '=');
        if (!eq) {
            log_warn("%s:%d: expected key = value", path, lineno);
            bad++;
            continue;
        }
        *eq = '\0';
        const char *key = trim(s);
        const char *val = trim(eq + 1);
        if (*section == '\0') {
            log_warn("%s:%d: key '%s' before any [section]", path, lineno, key);
            bad++;
            continue;
        }
        err[0] = '\0';
        int r = config_apply(c, section, key, val, err, sizeof(err));
        if (r < 0) {
            log_warn("%s:%d: %s (keeping default)", path, lineno, err);
            bad++;
        } else if (r == 0 && err[0]) {
            log_warn("%s:%d: %s", path, lineno, err);
        }
    }
    fclose(f);
    return bad;
}

int config_load(config *c, const char *explicit_path)
{
    config_defaults(c);
    if (explicit_path)
        return config_load_file(c, explicit_path) < 0 ? -1 : 0;
    if (access(CONFIG_DEFAULT_PATH, R_OK) == 0) {
        log_info("config: %s", CONFIG_DEFAULT_PATH);
        config_load_file(c, CONFIG_DEFAULT_PATH);
    } else if (access(CONFIG_FALLBACK_PATH, R_OK) == 0) {
        log_info("config: %s", CONFIG_FALLBACK_PATH);
        config_load_file(c, CONFIG_FALLBACK_PATH);
    } else {
        log_info("config: no file found, using built-in defaults");
    }
    return 0;
}
