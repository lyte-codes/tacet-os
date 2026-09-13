/* Press / release / repeat state machine (spec §4.2). Time is passed in, so
 * the tests drive it with a fake clock; main.c drives it from CLOCK_MONOTONIC
 * and a timerfd armed to repeat_next_deadline().
 *
 * One key is held at a time: a TV remote is a one-button-at-a-time device,
 * and holding a second key implicitly releases the first. */
#ifndef TACET_REPEAT_H
#define TACET_REPEAT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct repeat_config {
    int delay_ms;     /* first repeat after this */
    int rate_ms;      /* then every this */
    int timeout_ms;   /* safety release if no release arrives */
    int debounce_ms;  /* ignore a re-press of the same code within this */
} repeat_config;

typedef enum { RA_PRESS, RA_RELEASE, RA_REPEAT } repeat_action_kind;

typedef struct repeat_action {
    repeat_action_kind kind;
    uint8_t cec;
} repeat_action;

typedef struct repeat_state {
    repeat_config cfg;
    bool     held;
    uint8_t  held_cec;
    bool     held_no_repeat;
    int64_t  next_repeat_at;   /* valid when held && !held_no_repeat */
    int64_t  timeout_at;       /* valid when held */
    bool     have_last_press;
    uint8_t  last_press_cec;
    int64_t  last_press_at;
} repeat_state;

void repeat_init(repeat_state *s, const repeat_config *cfg);

/* Each returns the number of actions written to out (capacity 2). */
int  repeat_press(repeat_state *s, uint8_t cec, bool no_repeat, int64_t now_ms, repeat_action out[2]);
int  repeat_release(repeat_state *s, uint8_t cec, int64_t now_ms, repeat_action out[2]);
int  repeat_tick(repeat_state *s, int64_t now_ms, repeat_action out[2]);
int  repeat_release_all(repeat_state *s, repeat_action out[2]);

/* Absolute ms of the next tick needed, or -1 when idle. */
int64_t repeat_next_deadline(const repeat_state *s);

#endif
