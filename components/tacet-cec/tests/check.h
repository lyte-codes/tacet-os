/* Minimal assertion helpers for the tacet-cec unit tests. */
#ifndef TACET_CHECK_H
#define TACET_CHECK_H

#include <stdio.h>
#include <string.h>

extern int check_failures;
extern int check_passes;

#define CHECK(cond) do { \
    if (cond) check_passes++; \
    else { check_failures++; fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

#define CHECK_EQ(a, b) do { \
    long long _a = (long long)(a), _b = (long long)(b); \
    if (_a == _b) check_passes++; \
    else { check_failures++; fprintf(stderr, "FAIL %s:%d: %s == %s (%lld != %lld)\n", __FILE__, __LINE__, #a, #b, _a, _b); } \
} while (0)

#define CHECK_STR(a, b) do { \
    const char *_a = (a), *_b = (b); \
    if (_a && _b && strcmp(_a, _b) == 0) check_passes++; \
    else { check_failures++; fprintf(stderr, "FAIL %s:%d: %s == \"%s\" (got \"%s\")\n", __FILE__, __LINE__, #a, _b ? _b : "(null)", _a ? _a : "(null)"); } \
} while (0)

#endif
