# Status

Milestones from spec §10. "Done when" is the acceptance test; nothing here is
ticked without it.

| Milestone | State | Notes |
|---|---|---|
| **M0 — Boots** | in progress | Repo, Containerfile, tacet-session, overlays, CI and docs are written. Not yet built in CI or booted on an N100. |
| **M1 — Remote works** | blocked | `tacet-cec-bridge-spec.md` is not in the repo. |
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
- [ ] First green CI run (validates every package name against real repos)
- [ ] Boots to Kodi on an N100 from a flashed image ← **M0 done when**

## Known gaps to close before calling M0 done

- Package names come from memory of Fedora 44 / RPM Fusion; the first CI build
  is the proof. Likely suspects: `google-noto-sans-cjk-vf-fonts`,
  `plymouth-plugin-script`, `kodi-inputstream-adaptive`.
- Kodi under gamescope's XWayland is the documented path; Kodi's GBM backend
  is the fallback if HDR or mode switching misbehaves.
- The Jellyfin Kodi add-on is not bundled yet: it needs a pinned download
  (URL + sha256) in the Containerfile and a `docs/PRIVACY.md` note that the
  fetch is build-time only.
- No Kodi favourites for "Firefox" / "Settings" until those exist (M4).
- `bootc` signature verification (`/etc/containers/policy.json`) lands with
  M2; containers-policy cannot yet express a GitHub Actions keyless identity
  without a `subjectEmail`, so this may need a key-based cosign identity.
