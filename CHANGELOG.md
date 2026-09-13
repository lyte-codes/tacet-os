# Changelog

All notable changes to Tacet OS are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); entries are generated
from conventional commits at release time and curated by hand.

Version scheme: `<FEDORA>.<MAJOR>.<PATCH>` (spec §7).

## [Unreleased]

### Added
- Repository skeleton: Containerfile on Fedora bootc 44, overlays, packaging,
  CI workflows, docs (M0 groundwork).
- `tacet-session`: system unit that runs gamescope + Kodi on tty1 as the `tv`
  user, with `/etc/tacet/session.conf` for shell, resolution, refresh, HDR.
- `tacet-boot-report`: VM-only oneshot used by the QEMU boot test.
- `tacet-cec` 0.1.0: HDMI-CEC to uinput bridge (virtual keyboard + gamepad),
  INI config, hold-to-repeat with lost-release protection, adapter reconnect
  with backoff, hardened unit with no network access (M1).
- Tacet plymouth theme and kernel arguments.
- Privacy contract (`docs/PRIVACY.md`), hardware and flashing docs.
