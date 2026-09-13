# tacet-cec — CEC → uinput bridge

**Component:** Tacet novel code #1
**Status:** v0.1 spec, ready for implementation
**Target:** Fedora bootc image, x86 (Intel N100) first; Raspberry Pi 5 second

---

## 1. Purpose

Make the TV's own remote control work as a normal input device for every app on the box. The bridge listens for HDMI-CEC key presses from the TV, translates them into Linux input events via `uinput`, and exposes a virtual keyboard + gamepad that Kodi, gamescope, Firefox, and any future Tacet shell see as ordinary hardware.

Without this, users need a second remote or a keyboard. This is the single highest-impact piece of Tacet v1.

## 2. Non-goals (v0.1)

- No GUI. No settings UI. Config is a file.
- No CEC *sending* beyond what's required for the TV to route keys to us (see §5.3). No "turn TV on/off", volume control, or input switching from the box side.
- No multi-device CEC topology (AVRs, soundbars). Single TV, single box.
- No IR / Bluetooth remotes. Those already work through libinput.
- No per-app keymaps. One global map.

## 3. Language and dependencies

- **Language:** C (C11), matching the rest of Ricky's toolchain. Single binary, no runtime deps beyond libc + libcec.
- **CEC:** `libcec` (>= 6.x) via its C API (`cecc.h`). Do not use the C++ API or `cec-client` as a subprocess.
- **Input:** raw `/dev/uinput` ioctls via `<linux/uinput.h>`. Do not depend on `libevdev` for the uinput side (keep it minimal); `libevdev` may be used in tests only.
- **Config:** INI-style, parsed by hand or a single-header parser (`inih`) vendored in-tree.
- **Build:** plain `Makefile`. No CMake, no autotools. Must build cleanly with `-Wall -Wextra -Werror -std=c11`.
- **Packaging:** RPM spec file in-tree so the Containerfile can `dnf install` it from a COPR or local build.

## 4. Behaviour

### 4.1 Startup

1. Load config from `/etc/tacet/cec.conf`, falling back to `/usr/share/tacet/cec.conf.default` (shipped in image). Missing keys use built-in defaults.
2. Enumerate CEC adapters via libcec. If none found, log, sleep with exponential backoff (1s → 30s cap), retry forever. Do not exit; the HDMI link may come up late or the TV may be off.
3. Open the first adapter (or the one named in config). Register as **Playback Device 1** (logical address `0x04`) unless config overrides.
4. Create two uinput devices:
   - `Tacet CEC Keyboard` — key events (`EV_KEY`) for every keycode referenced by the keymap.
   - `Tacet CEC Gamepad` — `EV_KEY` for gamepad buttons + `EV_ABS` d-pad hat (`ABS_HAT0X/Y`). Created only if `gamepad = true` in config.
5. Send CEC "Active Source" so the TV routes remote keys to us (see §5.3).
6. Enter event loop.

### 4.2 Event loop

- libcec delivers key presses via callback (`cbKeyPress` / `cec_keypress`). Each carries a CEC user-control code and a duration (0 = press, >0 = release).
- Look up the code in the keymap. Unmapped codes are logged at debug level and dropped.
- Emit the mapped uinput event(s): key down on press, key up on release, then `EV_SYN`.
- **Repeat handling:** CEC repeat behaviour varies by TV. Implement our own: on press, start a repeat timer (`repeat_delay_ms`, default 400) then repeat every `repeat_rate_ms` (default 100) until release or until `repeat_timeout_ms` (default 2000) as a safety cap if the release is lost. Configurable per-key `no_repeat` for things like Select/Back.
- **Lost release protection:** if a release never arrives (common with some TVs), the safety cap above releases the key. Also release all held keys on adapter disconnect or shutdown.
- **Debounce:** ignore a press of the same code within `debounce_ms` (default 50) of the previous press.

### 4.3 Adapter loss and reconnection

- On libcec alert/disconnect: release all keys, close the adapter, return to startup step 2 (backoff retry). Keep uinput devices open so apps don't see hotplug churn.
- On TV power state change to standby: keep running; no action.
- On TV power on / active-source change: re-send Active Source if `reclaim_active_source = true` (default true).

### 4.4 Shutdown

- On SIGTERM/SIGINT: release all held keys, emit SYN, destroy uinput devices, close libcec, exit 0.

## 5. Key mapping

### 5.1 Default map (keyboard device)

CEC user-control code → Linux keycode. Covers the standard TV remote.

| CEC | Code | Keyboard | Notes |
|---|---|---|---|
| Select | 0x00 | KEY_ENTER | no_repeat |
| Up | 0x01 | KEY_UP | |
| Down | 0x02 | KEY_DOWN | |
| Left | 0x03 | KEY_LEFT | |
| Right | 0x04 | KEY_RIGHT | |
| Exit | 0x0D | KEY_ESC | no_repeat |
| Root Menu | 0x09 | KEY_HOME | no_repeat |
| Setup Menu | 0x0A | KEY_MENU | no_repeat |
| Contents Menu | 0x0B | KEY_MENU | no_repeat |
| Number 0–9 | 0x20–0x29 | KEY_0–KEY_9 | |
| Channel Up | 0x30 | KEY_PAGEUP | |
| Channel Down | 0x31 | KEY_PAGEDOWN | |
| Display Info | 0x35 | KEY_INFO | no_repeat |
| Page Up | 0x37 | KEY_PAGEUP | |
| Page Down | 0x38 | KEY_PAGEDOWN | |
| Play | 0x44 | KEY_PLAY | no_repeat |
| Stop | 0x45 | KEY_STOP | no_repeat |
| Pause | 0x46 | KEY_PAUSE | no_repeat |
| Rewind | 0x48 | KEY_REWIND | |
| Fast Forward | 0x49 | KEY_FASTFORWARD | |
| Forward (skip) | 0x4B | KEY_NEXTSONG | no_repeat |
| Backward (skip) | 0x4C | KEY_PREVIOUSSONG | no_repeat |
| Play/Pause (F1 blue etc. vary) | 0x60 | KEY_PLAYPAUSE | no_repeat |
| F1 Blue | 0x71 | KEY_BLUE | no_repeat |
| F2 Red | 0x72 | KEY_RED | no_repeat |
| F3 Green | 0x73 | KEY_GREEN | no_repeat |
| F4 Yellow | 0x74 | KEY_YELLOW | no_repeat |

Volume Up/Down/Mute (0x41/0x42/0x43) are **not mapped by default** — TVs handle their own volume and generally don't forward these. Mappable in config if a user wants box-side volume.

### 5.2 Gamepad device (optional, default on)

When enabled, d-pad and Select/Exit are *additionally* emitted as gamepad events so gamescope and controller-native apps behave correctly:

| CEC | Gamepad |
|---|---|
| Up/Down/Left/Right | ABS_HAT0Y ±1 / ABS_HAT0X ±1 |
| Select | BTN_SOUTH (A) |
| Exit | BTN_EAST (B) |
| Root Menu | BTN_MODE (Guide) |
| Play/Pause | BTN_START |

Config flag `gamepad_mirror = true` controls whether both devices fire, or only gamepad for these keys.

### 5.3 Active Source

Most TVs only forward remote keys to the device that is the current "active source." On startup and on TV wake, the bridge sends `Active Source` with its physical address. This is the only CEC command the bridge transmits. It is also the one thing most likely to fight with other HDMI devices — hence `reclaim_active_source` is configurable and the physical address can be pinned.

## 6. Configuration file

`/etc/tacet/cec.conf`, INI. All keys optional.

```ini
[cec]
adapter = auto            ; or /dev/cec0, or libcec adapter path
logical_address = playback1 ; playback1|playback2|playback3|recording1
physical_address = auto   ; or e.g. 1.0.0.0
device_name = Tacet
reclaim_active_source = true

[input]
gamepad = true
gamepad_mirror = true
repeat_delay_ms = 400
repeat_rate_ms = 100
repeat_timeout_ms = 2000
debounce_ms = 50

[keymap]
; CEC code (hex) = KEY_NAME[,no_repeat]
; overrides defaults; set a code to "none" to unmap
0x41 = KEY_VOLUMEUP
0x42 = KEY_VOLUMEDOWN
0x43 = KEY_MUTE,no_repeat
0x71 = none

[log]
level = info              ; error|warn|info|debug
```

Key names are the `KEY_*` / `BTN_*` identifiers from `linux/input-event-codes.h`; the parser must map name → number via a generated table (script in `tools/` that greps the header at build time).

## 7. Runtime integration

- **Binary:** `/usr/bin/tacet-cec`
- **Unit:** `tacet-cec.service`, system scope (needs `/dev/uinput` + `/dev/cec*`):
  - `After=systemd-udevd.service`, `WantedBy=multi-user.target`
  - `Restart=always`, `RestartSec=2`
  - Runs as dedicated user `tacet-cec` with `SupplementaryGroups=input`; udev rule grants that user rw on `/dev/uinput` and `/dev/cec*`
  - Hardening: `ProtectSystem=strict`, `ProtectHome=yes`, `PrivateTmp=yes`, `NoNewPrivileges=yes`, `DeviceAllow=/dev/uinput rw`, `DeviceAllow=char-cec rw`, `CapabilityBoundingSet=` (empty)
- **udev:** `/usr/lib/udev/rules.d/70-tacet-cec.rules` — sets group/mode on `/dev/uinput` and `/dev/cec*`, tags the virtual devices so libinput and gamescope pick them up.
- **Logging:** stderr → journald. Structured single-line messages. Debug level logs every CEC frame received.
- **No network access.** The unit should also carry `PrivateNetwork=yes` — this daemon has no reason to touch the network, and the privacy posture should be enforced at the unit level.

## 8. CLI

```
tacet-cec [--config PATH] [--log-level LEVEL] [--foreground]
tacet-cec --list-adapters
tacet-cec --dump-keymap        ; prints effective map after config merge
tacet-cec --test               ; prints incoming CEC codes + mapped keys to stdout without creating uinput devices
```

`--test` is the primary user-facing debug tool: "press every button on your remote, paste the output in the issue."

## 9. Error handling

- Config parse error: log with line number, fall back to defaults for that key, continue. Never refuse to start over config.
- `/dev/uinput` missing or permission denied: log clearly (name the udev rule), exit 78 (EX_CONFIG). systemd restart loop is acceptable here — it's a packaging bug, not a runtime condition.
- libcec init failure: treat as no-adapter, backoff retry (§4.1 step 2).
- Unknown key name in config: log, ignore that line.

## 10. Testing

- **Unit tests** (`make test`): config parser, keymap merge, key-name table, repeat state machine (simulated timers). No hardware.
- **Integration test**: a `tools/fake-cec.sh` that drives libcec's emulated adapter if available, else documented manual test using `--test` with a real TV.
- **Acceptance (manual, documented in `TESTING.md`):**
  1. Boot box, TV remote navigates Kodi with no other input device.
  2. Hold Down: key repeats at ~10/s after ~400ms; stops on release.
  3. Unplug HDMI, replug: remote works again within 5s, no key stuck.
  4. Power TV off and on: remote works without restarting the service.
  5. `journalctl -u tacet-cec` shows no errors over 24h idle.

## 11. Deliverables

```
tacet-cec/
  Makefile
  src/
    main.c          ; arg parsing, signal handling, main loop
    cec.c/.h        ; libcec wrapper: open, callbacks, active source, reconnect
    uinput.c/.h     ; device creation, emit, release-all
    keymap.c/.h     ; defaults, config merge, lookup
    repeat.c/.h     ; press/release/repeat state machine, timerfd-based
    config.c/.h     ; INI load
    log.c/.h
  tools/
    gen-keycodes.py ; generates keycode name table from linux headers
  tests/
  packaging/
    tacet-cec.spec
    tacet-cec.service
    70-tacet-cec.rules
    cec.conf.default
  README.md
  TESTING.md
```

## 12. Open questions for implementation

- libcec on Fedora: confirm `libcec-devel` is in the repos for the target Fedora release and links against the C API cleanly under `-Werror`.
- Pi 5: libcec uses the kernel `cec` framework there (`/dev/cec0`), not a USB adapter — confirm the adapter-detection path covers both.
- Whether gamescope needs the gamepad device to advertise as a specific known controller (Xbox-style vendor/product IDs) to be treated as a controller. Test; if so, set those IDs.

## 13. Definition of done (v0.1)

Fresh Tacet image on N100 → boots to Kodi → all standard remote buttons work, repeat feels native, survives HDMI/TV power cycling, `rpm -q tacet-cec` installed from the Containerfile, unit hardened as in §7, `make test` green in CI.
