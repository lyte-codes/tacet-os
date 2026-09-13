# tacet-cec

HDMI-CEC → uinput bridge. The TV's remote becomes a virtual keyboard (and
gamepad) that every app on the box sees as ordinary hardware. `SPEC.md` is
the authoritative design; this file is the implementation map.

```
TV remote ──CEC──▶ adapter ──libcec──▶ tacet-cec ──▶ /dev/uinput ──▶ Tacet CEC Keyboard
                                          │                          Tacet CEC Gamepad
                                          └─ config: /etc/tacet/cec.conf
                                             (else /usr/share/tacet/cec.conf.default)
```

## Layout

| Path | Role |
|---|---|
| `src/main.c` | CLI, signal handling (signalfd), event loop over a pipe from libcec, timerfd for repeats |
| `src/cec.c` | libcec C API: adapter discovery, open, callbacks → events, Active Source |
| `src/uinput.c` | the two virtual devices; emit and release |
| `src/keymap.c` | §5.1 defaults, §5.2 gamepad table, `[keymap]` merge, name tables |
| `src/repeat.c` | press / release / repeat / debounce / lost-release state machine, time passed in |
| `src/config.c` | hand-written INI parser for §6 |
| `src/log.c` | levels, one line per message on stderr |
| `src/keycodes.h` | generated at build by `tools/gen-keycodes.py` from `linux/input-event-codes.h` |
| `tests/` | unit tests for keymap, config and repeat; no hardware, no libcec |
| `tools/fake-cec.sh` | no-TV integration check (build, tests, backoff, clean shutdown) |
| `packaging/` | RPM spec, systemd unit, udev rules, modules-load, default config |

## Build and test

```
make            # needs libcec headers (libcec-devel / libcec-dev) and python3
make test       # no libcec needed
make lint       # -fsyntax-only on everything that can be compiled here
tools/fake-cec.sh
```

Runtime checks without a TV:

```
tacet-cec --list-adapters     # prints "no CEC adapters found" and exits 0
tacet-cec --dump-keymap       # effective map after config merge
tacet-cec --test              # what the TV sends and what it maps to (no uinput)
```

## Threads and timing

libcec calls back on its own threads. Those callbacks only write a 2-byte
event into a non-blocking pipe; the main thread owns all state and reads the
pipe from `poll()` alongside a signalfd and a timerfd. The repeat state
machine is fed monotonic milliseconds, so the tests drive it with a fake
clock and the daemon with `CLOCK_MONOTONIC` plus a timerfd armed to
`repeat_next_deadline()`.

## Decisions not spelled out by the spec

Grep for `DECISION:` in `src/` and `packaging/`. The important ones:

- **Repeats are release+press pairs**, not `EV_KEY` value-2 autorepeat
  events. libinput drops value-2 events from devices and compositors run
  their own repeat, so the configured `repeat_rate_ms` would otherwise have
  no effect. Re-pressing at our cadence (400 ms, then every 100 ms) keeps the
  compositor's slower repeat timer from ever firing.
- **Active Source is re-claimed on TV wake only**, not on every
  active-source change: re-claiming when the user switches the TV to another
  input would fight them for the screen. Losing active source releases any
  held key and is logged.
- **Pulse-Eight USB adapters** (`/dev/ttyACM*`) get a udev rule and a
  `DeviceAllow`; the spec only names `/dev/cec*`, but the N100 has no native
  CEC.
- `logical_address = playbackN` selects the device *type*; libcec allocates
  the first free playback address, so the number is a preference.

## Known limitation with Kodi under XWayland

X11 keycodes stop at 255, which is Linux code 247. The spec's defaults for
Display Info (`KEY_INFO`, 358) and the colour keys (`KEY_RED` … `KEY_BLUE`,
398–401) cannot reach an X11 client, and Kodi runs under gamescope's XWayland
in Tacet v1. Everything else in the default map fits. `cec.conf.default`
shows the one-line overrides (`0x35 = KEY_I` and F1–F4 for the colours) until
the shell moves to native Wayland. This is on the M1 hardware checklist in
`TESTING.md`.
