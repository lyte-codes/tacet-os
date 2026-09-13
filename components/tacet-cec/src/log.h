#ifndef TACET_LOG_H
#define TACET_LOG_H

typedef enum { LOG_ERROR = 0, LOG_WARN, LOG_INFO, LOG_DEBUG } log_level;

void        log_set_level(log_level level);
log_level   log_get_level(void);
int         log_level_from_name(const char *name);   /* -1 if unknown */
const char *log_level_name(log_level level);
void        log_msg(log_level level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#define log_error(...) log_msg(LOG_ERROR, __VA_ARGS__)
#define log_warn(...)  log_msg(LOG_WARN, __VA_ARGS__)
#define log_info(...)  log_msg(LOG_INFO, __VA_ARGS__)
#define log_debug(...) log_msg(LOG_DEBUG, __VA_ARGS__)

#endif
