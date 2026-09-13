#include "check.h"
#include "../src/repeat.h"

void test_repeat(void)
{
    repeat_config cfg = { .delay_ms = 400, .rate_ms = 100, .timeout_ms = 2000, .debounce_ms = 50 };
    repeat_state s;
    repeat_action a[2];

    /* Press, repeat after delay, then at rate, release stops it. */
    repeat_init(&s, &cfg);
    CHECK_EQ(repeat_next_deadline(&s), -1);
    CHECK_EQ(repeat_press(&s, 0x01, false, 0, a), 1);
    CHECK_EQ(a[0].kind, RA_PRESS); CHECK_EQ(a[0].cec, 0x01);
    CHECK_EQ(repeat_next_deadline(&s), 400);
    CHECK_EQ(repeat_tick(&s, 399, a), 0);
    CHECK_EQ(repeat_tick(&s, 400, a), 1);
    CHECK_EQ(a[0].kind, RA_REPEAT);
    CHECK_EQ(repeat_next_deadline(&s), 500);
    CHECK_EQ(repeat_tick(&s, 450, a), 0);
    CHECK_EQ(repeat_tick(&s, 500, a), 1);
    CHECK_EQ(repeat_tick(&s, 600, a), 1);
    CHECK_EQ(repeat_release(&s, 0x01, 650, a), 1);
    CHECK_EQ(a[0].kind, RA_RELEASE); CHECK_EQ(a[0].cec, 0x01);
    CHECK_EQ(repeat_next_deadline(&s), -1);
    CHECK_EQ(repeat_tick(&s, 700, a), 0);

    /* A late tick does not burst-repeat; cadence resumes from now. */
    repeat_init(&s, &cfg);
    repeat_press(&s, 0x02, false, 0, a);
    CHECK_EQ(repeat_tick(&s, 900, a), 1);
    CHECK_EQ(repeat_next_deadline(&s), 1000);

    /* no_repeat keys never repeat but still time out. */
    repeat_init(&s, &cfg);
    CHECK_EQ(repeat_press(&s, 0x00, true, 0, a), 1);
    CHECK_EQ(repeat_next_deadline(&s), 2000);
    CHECK_EQ(repeat_tick(&s, 400, a), 0);
    CHECK_EQ(repeat_tick(&s, 1999, a), 0);
    CHECK_EQ(repeat_tick(&s, 2000, a), 1);
    CHECK_EQ(a[0].kind, RA_RELEASE);
    CHECK_EQ(repeat_next_deadline(&s), -1);

    /* Lost release on a repeating key: timeout releases it. */
    repeat_init(&s, &cfg);
    repeat_press(&s, 0x01, false, 0, a);
    int repeats = 0;
    for (int64_t t = 0; t <= 2000; t += 100)
        if (repeat_tick(&s, t, a) == 1 && a[0].kind == RA_REPEAT) repeats++;
    CHECK_EQ(repeats, 16);   /* 400..1900 */
    CHECK(!s.held);

    /* Debounce: same code again within 50ms is ignored. */
    repeat_init(&s, &cfg);
    repeat_press(&s, 0x01, false, 0, a);
    repeat_release(&s, 0x01, 10, a);
    CHECK_EQ(repeat_press(&s, 0x01, false, 30, a), 0);
    CHECK(!s.held);
    CHECK_EQ(repeat_press(&s, 0x01, false, 60, a), 1);

    /* Repeated press of the held code (TVs that resend instead of hold)
     * emits nothing but extends the safety timeout. */
    repeat_init(&s, &cfg);
    repeat_press(&s, 0x01, false, 0, a);
    CHECK_EQ(repeat_press(&s, 0x01, false, 1500, a), 0);
    CHECK(s.held);
    CHECK_EQ(repeat_tick(&s, 2100, a), 1);   /* still repeating, not released */
    CHECK_EQ(a[0].kind, RA_REPEAT);
    CHECK_EQ(repeat_tick(&s, 3500, a), 1);
    CHECK_EQ(a[0].kind, RA_RELEASE);

    /* A different key while one is held releases the first. */
    repeat_init(&s, &cfg);
    repeat_press(&s, 0x01, false, 0, a);
    CHECK_EQ(repeat_press(&s, 0x02, false, 100, a), 2);
    CHECK_EQ(a[0].kind, RA_RELEASE); CHECK_EQ(a[0].cec, 0x01);
    CHECK_EQ(a[1].kind, RA_PRESS);   CHECK_EQ(a[1].cec, 0x02);
    /* Stray release of a key that is not held is ignored. */
    CHECK_EQ(repeat_release(&s, 0x01, 150, a), 0);
    CHECK(s.held);
    CHECK_EQ(repeat_release(&s, 0x02, 200, a), 1);

    /* release_all on shutdown / disconnect. */
    repeat_init(&s, &cfg);
    CHECK_EQ(repeat_release_all(&s, a), 0);
    repeat_press(&s, 0x03, false, 0, a);
    CHECK_EQ(repeat_release_all(&s, a), 1);
    CHECK_EQ(a[0].kind, RA_RELEASE); CHECK_EQ(a[0].cec, 0x03);
    CHECK_EQ(repeat_next_deadline(&s), -1);
}
