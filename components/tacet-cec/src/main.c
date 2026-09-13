/* tacet-cec: CEC -> uinput bridge. Spec: SPEC.md. */
#include "cec.h"
#include "config.h"
#include "keymap.h"
#include "log.h"
#include "repeat.h"
#include "uinput.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/input-event-codes.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>

#define BACKOFF_MIN_MS 1000
#define BACKOFF_MAX_MS 30000

typedef struct bridge {
    config       cfg;
    repeat_state rs;
    int          kb_fd;        /* -1 in --test mode */
    int          gp_fd;
    bool         test_mode;
    int          pipe_r, pipe_w;
    int          sig_fd;
    int          timer_fd;
    cec_conn    *cec;
} bridge;

static int64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* libcec thread -> main loop. A cec_event is far below PIPE_BUF, so the
 * write is atomic; O_NONBLOCK means a stalled main loop drops events rather
 * than blocking libcec. */
static void on_cec_event(void *user, const cec_event *ev)
{
    bridge *b = user;
    if (write(b->pipe_w, ev, sizeof(*ev)) != (ssize_t)sizeof(*ev))
        log_warn("event queue full, dropped a CEC event");
}

static const char *cec_name_or_hex(uint8_t code, char *buf, size_t len)
{
    const char *n = keymap_cec_name(code);
    if (n)
        return n;
    snprintf(buf, len, "0x%02X", code);
    return buf;
}

static void emit_gamepad(bridge *b, gamepad_action g, int pressed)
{
    if (b->gp_fd < 0 || g.kind == GP_NONE)
        return;
    if (g.kind == GP_BUTTON)
        uinput_key(b->gp_fd, (uint16_t)g.value, pressed);
    else
        uinput_abs(b->gp_fd, g.kind == GP_HAT_X ? ABS_HAT0X : ABS_HAT0Y, pressed ? g.value : 0);
    uinput_syn(b->gp_fd);
}

static void apply_action(bridge *b, const repeat_action *a)
{
    const keymap_entry *e = keymap_lookup(&b->cfg.map, a->cec);
    gamepad_action g = b->cfg.gamepad ? keymap_gamepad(a->cec) : (gamepad_action){ GP_NONE, 0 };
    bool use_kb = e->key != KEYMAP_UNMAPPED && (b->cfg.gamepad_mirror || g.kind == GP_NONE);
    char hex[8];
    const char *kind = a->kind == RA_PRESS ? "press" : a->kind == RA_RELEASE ? "release" : "repeat";

    if (b->test_mode) {
        printf("%-7s 0x%02X %-24s -> %s%s\n", kind, a->cec, cec_name_or_hex(a->cec, hex, sizeof(hex)),
               use_kb ? keycode_to_name(e->key) : "(keyboard: none)",
               g.kind == GP_NONE ? "" : g.kind == GP_BUTTON ? " + gamepad button" : " + gamepad hat");
        fflush(stdout);
        return;
    }
    log_debug("%s 0x%02X %s -> %s", kind, a->cec, cec_name_or_hex(a->cec, hex, sizeof(hex)),
              use_kb ? keycode_to_name(e->key) : "-");

    switch (a->kind) {
    case RA_PRESS:
        if (use_kb) { uinput_key(b->kb_fd, e->key, 1); uinput_syn(b->kb_fd); }
        emit_gamepad(b, g, 1);
        break;
    case RA_RELEASE:
        if (use_kb) { uinput_key(b->kb_fd, e->key, 0); uinput_syn(b->kb_fd); }
        emit_gamepad(b, g, 0);
        break;
    case RA_REPEAT:
        /* DECISION: a repeat is a release+press pair rather than an EV_KEY
         * value-2 autorepeat event. libinput discards value-2 events from
         * devices and compositors run their own repeat, which would make
         * repeat_rate_ms meaningless. Re-pressing at our cadence keeps the
         * compositor's own repeat timer from ever firing (its delay is longer
         * than our period), so the configured rate is what apps see. Hats are
         * state, not edges, so the gamepad side is left held. */
        if (use_kb) {
            uinput_key(b->kb_fd, e->key, 0); uinput_syn(b->kb_fd);
            uinput_key(b->kb_fd, e->key, 1); uinput_syn(b->kb_fd);
        }
        break;
    }
}

static void apply_actions(bridge *b, const repeat_action *acts, int n)
{
    for (int i = 0; i < n; i++)
        apply_action(b, &acts[i]);
}

static void arm_timer(bridge *b)
{
    struct itimerspec its;
    memset(&its, 0, sizeof(its));
    int64_t dl = repeat_next_deadline(&b->rs);
    if (dl >= 0) {
        its.it_value.tv_sec = dl / 1000;
        its.it_value.tv_nsec = (dl % 1000) * 1000000;
        if (its.it_value.tv_sec == 0 && its.it_value.tv_nsec == 0)
            its.it_value.tv_nsec = 1;
    }
    timerfd_settime(b->timer_fd, TFD_TIMER_ABSTIME, &its, NULL);
}

static void release_all(bridge *b)
{
    repeat_action acts[2];
    apply_actions(b, acts, repeat_release_all(&b->rs, acts));
    arm_timer(b);
}

/* Returns true if a termination signal arrived. */
static bool wait_ms(bridge *b, int ms)
{
    struct pollfd p = { .fd = b->sig_fd, .events = POLLIN };
    return poll(&p, 1, ms) > 0;
}

static void handle_cec_event(bridge *b, const cec_event *ev, bool *disconnected)
{
    repeat_action acts[2];
    char hex[8];
    switch (ev->kind) {
    case CEC_EV_KEY_PRESS: {
        const keymap_entry *e = keymap_lookup(&b->cfg.map, ev->code);
        bool gp = b->cfg.gamepad && keymap_gamepad(ev->code).kind != GP_NONE;
        if (e->key == KEYMAP_UNMAPPED && !gp) {
            if (b->test_mode)
                printf("press   0x%02X %-24s -> unmapped\n", ev->code, cec_name_or_hex(ev->code, hex, sizeof(hex)));
            else
                log_debug("unmapped CEC code 0x%02X %s", ev->code, cec_name_or_hex(ev->code, hex, sizeof(hex)));
            return;
        }
        apply_actions(b, acts, repeat_press(&b->rs, ev->code, e->no_repeat, now_ms(), acts));
        break;
    }
    case CEC_EV_KEY_RELEASE:
        apply_actions(b, acts, repeat_release(&b->rs, ev->code, now_ms(), acts));
        break;
    case CEC_EV_DISCONNECT:
        *disconnected = true;
        break;
    case CEC_EV_TV_WAKE:
        log_info("TV is awake");
        if (b->cfg.reclaim_active_source && b->cec) {
            if (cec_set_active_source(b->cec) < 0)
                log_warn("could not re-claim active source");
            else
                log_info("re-claimed active source");
        }
        break;
    case CEC_EV_TV_STANDBY:
        log_info("TV went to standby");
        release_all(b);
        break;
    case CEC_EV_SOURCE_LOST:
        /* DECISION: spec §4.3 says re-send Active Source on "active-source
         * change"; doing that when the user switches the TV to another HDMI
         * input would fight them for the screen. We only re-claim on TV wake
         * (CEC_EV_TV_WAKE). Log so the behaviour is visible. */
        release_all(b);
        break;
    }
}

/* Runs until the adapter drops or a signal arrives. Returns true on signal. */
static bool run_connected(bridge *b)
{
    struct pollfd fds[3] = {
        { .fd = b->pipe_r, .events = POLLIN },
        { .fd = b->sig_fd, .events = POLLIN },
        { .fd = b->timer_fd, .events = POLLIN },
    };
    bool disconnected = false;
    while (!disconnected) {
        arm_timer(b);
        if (poll(fds, 3, -1) < 0) {
            if (errno == EINTR)
                continue;
            log_error("poll: %s", strerror(errno));
            return true;
        }
        if (fds[1].revents)
            return true;
        if (fds[2].revents) {
            uint64_t expirations;
            if (read(b->timer_fd, &expirations, sizeof(expirations)) < 0 && errno != EAGAIN)
                log_warn("timerfd read: %s", strerror(errno));
            repeat_action acts[2];
            apply_actions(b, acts, repeat_tick(&b->rs, now_ms(), acts));
        }
        if (fds[0].revents) {
            cec_event ev;
            while (read(b->pipe_r, &ev, sizeof(ev)) == (ssize_t)sizeof(ev))
                handle_cec_event(b, &ev, &disconnected);
        }
    }
    return false;
}

static int run(bridge *b)
{
    cec_settings cs = {
        .adapter = b->cfg.adapter,
        .device_type = b->cfg.logical_address == LA_RECORDING1 ? 1 : 4,
        .physical_address = b->cfg.physical_address,
        .device_name = b->cfg.device_name,
        .activate_source = true,   /* spec §4.1 step 5 */
    };
    char err[256], last_err[256] = "";
    int delay = BACKOFF_MIN_MS;

    for (;;) {
        b->cec = cec_open(&cs, on_cec_event, b, err, sizeof(err));
        if (!b->cec) {
            if (strcmp(err, last_err) != 0) {
                log_warn("%s; retrying (backoff up to %ds)", err, BACKOFF_MAX_MS / 1000);
                snprintf(last_err, sizeof(last_err), "%s", err);
            } else {
                log_debug("%s; retrying in %dms", err, delay);
            }
            if (wait_ms(b, delay))
                return 0;
            delay = delay * 2 > BACKOFF_MAX_MS ? BACKOFF_MAX_MS : delay * 2;
            continue;
        }
        delay = BACKOFF_MIN_MS;
        last_err[0] = '\0';
        log_info("connected to %s, logical address %X, active source requested",
                 cec_port(b->cec), (unsigned)cec_conn_logical_address(b->cec));

        bool quit = run_connected(b);
        release_all(b);
        cec_close(b->cec);
        b->cec = NULL;
        if (quit)
            return 0;
        log_warn("adapter disconnected, reconnecting");
        /* Drain anything queued by the old connection. */
        cec_event ev;
        while (read(b->pipe_r, &ev, sizeof(ev)) == (ssize_t)sizeof(ev))
            ;
    }
}

static void usage(FILE *f)
{
    fprintf(f,
        "usage: tacet-cec [--config PATH] [--log-level LEVEL] [--foreground]\n"
        "       tacet-cec --list-adapters\n"
        "       tacet-cec --dump-keymap\n"
        "       tacet-cec --test\n"
        "\n"
        "  --config PATH      INI config (default %s, else %s)\n"
        "  --log-level LEVEL  error|warn|info|debug (overrides [log] level)\n"
        "  --foreground       accepted for compatibility; the bridge never daemonises\n"
        "  --list-adapters    print CEC adapters libcec can see and exit\n"
        "  --dump-keymap      print the effective keymap after config merge and exit\n"
        "  --test             print CEC codes and mapped keys to stdout, no uinput devices\n",
        CONFIG_DEFAULT_PATH, CONFIG_FALLBACK_PATH);
}

int main(int argc, char **argv)
{
    static const struct option opts[] = {
        { "config", required_argument, NULL, 'c' },
        { "log-level", required_argument, NULL, 'l' },
        { "foreground", no_argument, NULL, 'f' },
        { "list-adapters", no_argument, NULL, 'L' },
        { "dump-keymap", no_argument, NULL, 'D' },
        { "test", no_argument, NULL, 'T' },
        { "version", no_argument, NULL, 'V' },
        { "help", no_argument, NULL, 'h' },
        { 0, 0, 0, 0 },
    };
    const char *config_path = NULL, *level_arg = NULL;
    bool list = false, dump = false, test = false;
    int opt;
    while ((opt = getopt_long(argc, argv, "c:l:fh", opts, NULL)) != -1) {
        switch (opt) {
        case 'c': config_path = optarg; break;
        case 'l': level_arg = optarg; break;
        case 'f': break;
        case 'L': list = true; break;
        case 'D': dump = true; break;
        case 'T': test = true; break;
        case 'V': printf("tacet-cec %s\n", TACET_CEC_VERSION); return 0;
        case 'h': usage(stdout); return 0;
        default: usage(stderr); return EX_USAGE;
        }
    }

    static bridge b;
    b.kb_fd = b.gp_fd = -1;
    b.test_mode = test;
    if (config_load(&b.cfg, config_path) < 0)
        return EX_CONFIG;
    log_set_level(b.cfg.level);
    if (level_arg) {
        int l = log_level_from_name(level_arg);
        if (l < 0) {
            log_error("unknown log level '%s'", level_arg);
            return EX_USAGE;
        }
        log_set_level((log_level)l);
    }

    if (list)
        return cec_list_adapters(stdout) < 0 ? 1 : 0;
    if (dump) {
        keymap_dump(&b.cfg.map, b.cfg.gamepad, stdout);
        return 0;
    }

    repeat_config rc = {
        .delay_ms = b.cfg.repeat_delay_ms, .rate_ms = b.cfg.repeat_rate_ms,
        .timeout_ms = b.cfg.repeat_timeout_ms, .debounce_ms = b.cfg.debounce_ms,
    };
    repeat_init(&b.rs, &rc);

    if (!test) {
        char err[256];
        uint16_t keys[KEYMAP_CEC_CODES];
        size_t n = keymap_referenced_keys(&b.cfg.map, keys, KEYMAP_CEC_CODES);
        b.kb_fd = uinput_open_keyboard("Tacet CEC Keyboard", keys, n, err, sizeof(err));
        if (b.kb_fd < 0) {
            log_error("cannot create virtual keyboard: %s (is the tacet-cec user in group "
                      "'input' and 70-tacet-cec.rules installed?)", err);
            return EX_CONFIG;
        }
        if (b.cfg.gamepad) {
            b.gp_fd = uinput_open_gamepad("Tacet CEC Gamepad", err, sizeof(err));
            if (b.gp_fd < 0) {
                log_error("cannot create virtual gamepad: %s", err);
                uinput_close(b.kb_fd);
                return EX_CONFIG;
            }
        }
        log_info("virtual keyboard (%zu keys)%s created", n, b.cfg.gamepad ? " and gamepad" : "");
    } else {
        printf("# tacet-cec --test: press remote buttons; Ctrl-C to stop\n");
    }

    int pipefd[2];
    if (pipe2(pipefd, O_NONBLOCK | O_CLOEXEC) < 0) {
        log_error("pipe: %s", strerror(errno));
        return 1;
    }
    b.pipe_r = pipefd[0];
    b.pipe_w = pipefd[1];

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    b.sig_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    b.timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (b.sig_fd < 0 || b.timer_fd < 0) {
        log_error("signalfd/timerfd: %s", strerror(errno));
        return 1;
    }

    int rc_exit = run(&b);
    log_info("shutting down");
    release_all(&b);
    uinput_close(b.gp_fd);
    uinput_close(b.kb_fd);
    return rc_exit;
}
