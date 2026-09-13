/* Single-line messages on stderr; journald adds the timestamps. */
#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <strings.h>

static log_level current = LOG_INFO;
static const char *const names[] = { "error", "warn", "info", "debug" };

void log_set_level(log_level level) { current = level; }
log_level log_get_level(void) { return current; }

int log_level_from_name(const char *name)
{
    for (int i = 0; i < 4; i++)
        if (strcasecmp(name, names[i]) == 0)
            return i;
    if (strcasecmp(name, "warning") == 0)
        return LOG_WARN;
    return -1;
}

const char *log_level_name(log_level level)
{
    return names[level <= LOG_DEBUG ? level : LOG_DEBUG];
}

void log_msg(log_level level, const char *fmt, ...)
{
    if (level > current)
        return;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "tacet-cec: %s: ", names[level]);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}
