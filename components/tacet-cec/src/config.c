#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

void config_defaults(config *c)
{
    memset(c, 0, sizeof(*c));
    snprintf(c->device_name, sizeof(c->device_name), "Tacet");
    c->device_type = CONFIG_DEVICE_PLAYBACK;
    /* DECISION: the box claims the TV input when it starts, like every
     * consumer media player does. Without it the TV keeps routing the remote
     * to itself and "the TV's remote works everywhere" (spec §1) fails. */
    c->activate_source = true;
    /* DECISION: the box never powers the TV on by itself. */
    c->wake_tv = false;
    c->hdmi_port = 0;
    snprintf(c->adapter, sizeof(c->adapter), "auto");
    snprintf(c->keymap, sizeof(c->keymap), "/etc/tacet/cec-keymap.conf");
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

static int parse_bool(const char *v, bool *out)
{
    static const char *yes[] = { "yes", "true", "on", "1" };
    static const char *no[]  = { "no", "false", "off", "0" };
    for (size_t i = 0; i < 4; i++) {
        if (strcasecmp(v, yes[i]) == 0) { *out = true;  return 0; }
        if (strcasecmp(v, no[i])  == 0) { *out = false; return 0; }
    }
    return -1;
}

int config_parse_line(config *c, const char *line, char *err, size_t errlen)
{
    char buf[512];
    snprintf(buf, sizeof(buf), "%s", line);
    char *s = trim(buf);
    if (*s == '\0' || *s == '#')
        return 0;
    char *eq = strchr(s, '=');
    if (!eq) {
        snprintf(err, errlen, "malformed line '%s'", s);
        return -1;
    }
    *eq = '\0';
    const char *key = trim(s);
    const char *val = trim(eq + 1);

    if (strcmp(key, "DEVICE_NAME") == 0) {
        size_t n = strlen(val);
        if (n == 0 || n > CONFIG_DEVICE_NAME_MAX) {
            snprintf(err, errlen, "DEVICE_NAME must be 1..%d characters", CONFIG_DEVICE_NAME_MAX);
            return -1;
        }
        snprintf(c->device_name, sizeof(c->device_name), "%s", val);
        return 1;
    }
    if (strcmp(key, "DEVICE_TYPE") == 0) {
        if (strcasecmp(val, "playback") == 0)  { c->device_type = CONFIG_DEVICE_PLAYBACK;  return 1; }
        if (strcasecmp(val, "recording") == 0) { c->device_type = CONFIG_DEVICE_RECORDING; return 1; }
        snprintf(err, errlen, "DEVICE_TYPE must be playback or recording");
        return -1;
    }
    if (strcmp(key, "ACTIVATE_SOURCE") == 0 || strcmp(key, "WAKE_TV") == 0) {
        bool b;
        if (parse_bool(val, &b) < 0) {
            snprintf(err, errlen, "%s must be yes or no", key);
            return -1;
        }
        if (key[0] == 'A') c->activate_source = b; else c->wake_tv = b;
        return 1;
    }
    if (strcmp(key, "HDMI_PORT") == 0) {
        char *end;
        errno = 0;
        long p = (strcasecmp(val, "auto") == 0) ? 0 : strtol(val, &end, 10);
        if (strcasecmp(val, "auto") != 0 && (errno || *end || p < 1 || p > 15)) {
            snprintf(err, errlen, "HDMI_PORT must be auto or 1..15");
            return -1;
        }
        c->hdmi_port = (int)p;
        return 1;
    }
    if (strcmp(key, "ADAPTER") == 0 || strcmp(key, "KEYMAP") == 0) {
        if (*val == '\0' || strlen(val) >= CONFIG_PATH_MAX) {
            snprintf(err, errlen, "%s must be a path under %d characters", key, CONFIG_PATH_MAX);
            return -1;
        }
        snprintf(key[0] == 'A' ? c->adapter : c->keymap, CONFIG_PATH_MAX, "%s", val);
        return 1;
    }
    snprintf(err, errlen, "ignoring unknown key '%s'", key);
    return 0;
}

int config_load_file(config *c, const char *path, char *err, size_t errlen)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        snprintf(err, errlen, "%s: %s", path, strerror(errno));
        return -1;
    }
    char line[512], lerr[256];
    int bad = 0, lineno = 0;
    while (fgets(line, sizeof(line), f)) {
        lineno++;
        lerr[0] = '\0';
        int r = config_parse_line(c, line, lerr, sizeof(lerr));
        if (r < 0) {
            bad++;
            snprintf(err, errlen, "%s:%d: %s", path, lineno, lerr);
        } else if (r == 0 && lerr[0]) {
            fprintf(stderr, "tacet-cec: %s:%d: %s\n", path, lineno, lerr);
        }
    }
    fclose(f);
    return bad;
}
