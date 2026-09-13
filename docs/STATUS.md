# Status

Milestones from spec §10. "Done when" is the acceptance test; nothing here is
ticked without it.

| Milestone | State | Notes |
|---|---|---|
| **M0 — Boots** | in progress | CI is green: image builds, bootc-image-builder disk builds, QEMU boot test reports `session=active`. Not yet booted on an N100. |
| **M1 — Remote works** | in progress | tacet-cec implemented to `components/tacet-cec/SPEC.md`, unit-tested, in the image. Needs a TV: see its `TESTING.md`. |
| M2 — Updates | not started | Waits for M1 on hardware. |
| M3 — Silent | not started | |
| M4 — Usable | not started | |
| M5 — 1.0 | not started | |

## M0 checklist

- [x] Containerfile on `quay.io/fedora/fedora-bootc:44`, two-stage (RPM build + image)
- [x] Package list with categories; `docs/PACKAGES.md` generated and CI-checked
- [x] Telemetry removal: countme, abrt, PackageKit, cockpit, rsyslog, desktop sessions
- [x] Kernel arguments via `/usr/lib/bootc/kargs.d`
- [x] Users `tv` (1000), `tacet-cec`, group `tacet` via sysusers.d
- [x] `tacet-session`: unit + script + tests + RPM
- [x] Plymouth theme + initramfs rebuild
- [x] Kodi seed config (Estuary, no RSS, no add-on update checks, official repo disabled)
- [x] firewalld `tacet` zone as default; NetworkManager connectivity check off
- [x] CI: lint, tests, image build, bootc-image-builder disk, QEMU boot test
- [x] Release and weekly-rebuild workflows; version/tag tooling
- [x] First green CI run: image build ~5 min, disk ~3 min, QEMU boot to `tacet-session` active in ~15 s
- [ ] Boots to Kodi on an N100 from a flashed image ← **M0 done when**

## M1 checklist

- [x] `components/tacet-cec/SPEC.md` (v0.1) in the repo
- [x] C11 daemon: libcec C API, raw uinput keyboard + gamepad, INI config with
      `cec.conf.default` fallback, repeat/debounce/lost-release state machine
      on timerfd, backoff reconnect, `--list-adapters` / `--dump-keymap` / `--test`
- [x] Unit tests (keymap, config, repeat) and `tools/fake-cec.sh`, no hardware
- [x] Hardened unit (`PrivateNetwork=yes`, `DeviceAllow` only), udev rules,
      modules-load, RPM built in the Containerfile, service enabled
- [x] Boot test runs `tacet-cec --list-adapters` in the image
- [ ] TESTING.md acceptance list on a TV (N100 + Pulse-Eight, or Pi 5) ← **M1 done when**
- [ ] Decide on Info / colour keys under XWayland (README known limitation)
- [ ] Gamepad recognised by gamescope, or switch to Xbox-style IDs (spec §12)

## Known gaps to close before calling M0 done

- Every package name in the Containerfile resolved against Fedora 44 + RPM
  Fusion in CI except `blocky`, which is not packaged; tacet-dns (M3) has to
  source it.
- Kodi under gamescope's XWayland is the documented path; Kodi's GBM backend
  is the fallback if HDR or mode switching misbehaves.
- The Jellyfin Kodi add-on is not bundled yet: it needs a pinned download
  (URL + sha256) in the Containerfile and a `docs/PRIVACY.md` note that the
  fetch is build-time only.
- No Kodi favourites for "Firefox" / "Settings" until those exist (M4).
- `bootc` signature verification (`/etc/containers/policy.json`) lands with
  M2; containers-policy cannot yet express a GitHub Actions keyless identity
  without a `subjectEmail`, so this may need a key-based cosign identity.
