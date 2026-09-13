#include "repeat.h"

#include <string.h>

void repeat_init(repeat_state *s, const repeat_config *cfg)
{
    memset(s, 0, sizeof(*s));
    s->cfg = *cfg;
}

static int release_held(repeat_state *s, repeat_action *out)
{
    if (!s->held)
        return 0;
    out->kind = RA_RELEASE;
    out->cec = s->held_cec;
    s->held = false;
    return 1;
}

int repeat_press(repeat_state *s, uint8_t cec, bool no_repeat, int64_t now_ms, repeat_action out[2])
{
    if (s->have_last_press && s->last_press_cec == cec &&
        now_ms - s->last_press_at < s->cfg.debounce_ms)
        return 0;   /* debounced */

    int n = 0;
    if (s->held && s->held_cec == cec) {
        /* The TV re-sent the press while we still hold the key (some TVs
         * send repeated presses instead of one press + one release). Treat it
         * as "still held": push the safety timeout out, emit nothing. */
        s->timeout_at = now_ms + s->cfg.timeout_ms;
        s->last_press_at = now_ms;
        return 0;
    }
    n += release_held(s, &out[n]);
    out[n].kind = RA_PRESS;
    out[n].cec = cec;
    n++;
    s->held = true;
    s->held_cec = cec;
    s->held_no_repeat = no_repeat;
    s->next_repeat_at = now_ms + s->cfg.delay_ms;
    s->timeout_at = now_ms + s->cfg.timeout_ms;
    s->have_last_press = true;
    s->last_press_cec = cec;
    s->last_press_at = now_ms;
    return n;
}

int repeat_release(repeat_state *s, uint8_t cec, int64_t now_ms, repeat_action out[2])
{
    (void)now_ms;
    if (!s->held || s->held_cec != cec)
        return 0;   /* stray release: nothing held, or a different key */
    return release_held(s, &out[0]);
}

int repeat_tick(repeat_state *s, int64_t now_ms, repeat_action out[2])
{
    if (!s->held)
        return 0;
    if (now_ms >= s->timeout_at)
        return release_held(s, &out[0]);   /* lost release protection */
    if (s->held_no_repeat || now_ms < s->next_repeat_at)
        return 0;
    out[0].kind = RA_REPEAT;
    out[0].cec = s->held_cec;
    /* Schedule from the deadline, not from now, so a late tick does not
     * drift the cadence. */
    s->next_repeat_at += s->cfg.rate_ms;
    if (s->next_repeat_at <= now_ms)
        s->next_repeat_at = now_ms + s->cfg.rate_ms;
    return 1;
}

int repeat_release_all(repeat_state *s, repeat_action out[2])
{
    return release_held(s, &out[0]);
}

int64_t repeat_next_deadline(const repeat_state *s)
{
    if (!s->held)
        return -1;
    if (s->held_no_repeat)
        return s->timeout_at;
    return s->next_repeat_at < s->timeout_at ? s->next_repeat_at : s->timeout_at;
}
