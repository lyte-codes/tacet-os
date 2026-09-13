# tacet-session

Starts the TV session: gamescope on the seat, the configured shell inside it.
Spec §4.2.

## What it does

`tacet-session.service` is a system unit that runs `/usr/libexec/tacet/tacet-session`
as user `tv` on `/dev/tty1` with `PAMName=login`. PAM registers a logind seat
session, which is what gives gamescope access to the DRM device and input
devices without root. The script:

1. reads `/etc/tacet/session.conf` (KEY=VALUE; unknown keys ignored, invalid
   values fall back to defaults and are logged);
2. seeds `~/.kodi/userdata/` from `/usr/share/tacet/kodi/` on the very first
   launch, never afterwards;
3. `exec`s `gamescope -f [-W -H -w -h] [-r] [--hdr-enabled] -- <shell>`.

Crash handling is systemd's: `Restart=always`, `RestartSec=2`, no start limit.
HDMI hotplug is gamescope's: its DRM backend re-reads connectors on change.

`tacet-boot-report.service` is a oneshot that only runs in a VM
(`ConditionVirtualization=vm`). It waits for the session to be active, probes
`tacet-cec --list-adapters` if the binary exists, and writes one
`TACET-BOOT-REPORT ...` line to `/dev/ttyS0` for `ci/boot-test.sh`.

## Configuration

`/etc/tacet/session.conf`:

| Key | Values | Default |
|---|---|---|
| `SHELL` | `kodi`, `mpv`, `custom` | `kodi` |
| `SHELL_CMD` | command line, used when `SHELL=custom` | empty |
| `RESOLUTION` | `auto` or `WxH` | `auto` |
| `REFRESH` | `auto` or Hz | `auto` |
| `HDR` | `on`, `off` | `off` |
| `GAMESCOPE_ARGS` | extra gamescope flags | empty |

## Testing

```
make test    # dry-run the script against config fixtures
make lint    # shellcheck
```

Environment overrides used by the tests: `TACET_SESSION_CONF`,
`TACET_KODI_SEED_DIR`, `TACET_SESSION_DRY_RUN=1` (print instead of exec).

## Not yet verified on hardware

- Kodi under gamescope's XWayland (`--windowing=x11`). If Kodi's own GBM
  backend turns out to be better for HDR passthrough, `SHELL_CMD` can bypass
  gamescope entirely and this README changes.
- SELinux: the unit runs unconfined (`unconfined_service_t`); if enforcing
  mode denies DRM/input access, that is the first thing to look at in
  `journalctl -u tacet-session`.
