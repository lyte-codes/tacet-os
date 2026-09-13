#ifndef TACET_LOG_H
#define TACET_LOG_H

#include <stdio.h>

extern int log_verbose;

#define log_error(...) do { fprintf(stderr, "tacet-cec: error: " __VA_ARGS__); fputc('\n', stderr); } while (0)
#define log_warn(...)  do { fprintf(stderr, "tacet-cec: warning: " __VA_ARGS__); fputc('\n', stderr); } while (0)
#define log_info(...)  do { fprintf(stderr, "tacet-cec: " __VA_ARGS__); fputc('\n', stderr); } while (0)
#define log_debug(...) do { if (log_verbose) { fprintf(stderr, "tacet-cec: debug: " __VA_ARGS__); fputc('\n', stderr); } } while (0)

#endif
