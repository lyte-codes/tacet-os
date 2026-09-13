# Tacet OS — System Specification

**Status:** v0.1 spec
**Owner:** Ricky
**Implementation:** Claude Code, component by component
**One-line:** A privacy-first TV operating system. Flash it to a small box, plug it into a dumb (or dumbed-down) TV, and nothing phones home.

---

## 1. Goals

1. **Silent by default.** Zero outbound network traffic that the user did not cause. No telemetry, no crash reporting, no connectivity probes to third parties, no update checks without consent.
2. **Appliance reliability.** Boots to a usable screen in under 15s. Updates are atomic with automatic rollback. A power cut can never brick it.
3. **TV-native input.** The TV's own remote works everywhere via HDMI-CEC. No second remote required.
4. **Auditable.** Reproducible image builds, published SBOM, signed images, every network endpoint documented.
5. **Boring foundations.** Fedora bootc + systemd + Wayland. No dependency on a small-team downstream that may stop.

### Non-goals

- Native 4K DRM streaming (Netflix/Disney+ at L1). Not possible without certification. Browser Widevine L3 (≤1080p) is the ceiling and is documented as such.
- Replacing the TV's firmware. Tacet runs on an external box.
- Supporting every ARM board. One or two blessed targets.
- Being a general Linux desktop. No mouse-first apps, no package manager exposed to the user.

## 2. Hardware targets

| Tier | Device | Status |
|---|---|---|
| 1 | Intel N100 mini PC (any; reference: a common 8GB/256GB unit) | Primary. UEFI, full VA-API, mainline kernel. |
| 2 | Raspberry Pi 5 (4GB+) | Secondary. Blocked on bootc ARM boot chain maturity; tracked as an issue. |
| 3 | RISC-V (riscv64) | Planned, not started. Blocked on a board with mainline hardware video decode and a usable Fedora riscv64 bootc base. CI matrix line present, non-blocking. |
| — | Anything else | Community, unsupported. |

Requirements for any target: UEFI or bootc-supported boot, hardware H.264/HEVC/AV1 decode with an open driver, HDMI-CEC reachable from Linux (kernel `cec` framework or Pulse-Eight USB adapter).

## 3. Architecture

```
┌──────────────────────────────────────────────────────────┐
│  Apps: Kodi (v1 shell) · mpv · Firefox (TV profile) ·    │
│        Flatpak (curated)                                 │
├──────────────────────────────────────────────────────────┤
│  Tacet layer: tacet-session · tacet-cec · tacet-setup ·  │
│               tacet-settings · tacet-update · tacet-dns  │
├──────────────────────────────────────────────────────────┤
│  Compositor: gamescope (kiosk mode)                      │
├──────────────────────────────────────────────────────────┤
│  Fedora bootc: systemd · NetworkManager · PipeWire ·     │
│                firewalld · mesa · kernel                  │
├──────────────────────────────────────────────────────────┤
│  Hardware: N100 / Pi 5                                   │
└──────────────────────────────────────────────────────────┘
```

**Everything above the Fedora line is a Containerfile layer.** The Tacet repo is a Containerfile, config overlays, and a handful of small daemons, each in its own subdirectory with its own RPM.

## 4. Components

### 4.1 Base image (`Containerfile`)

- `FROM quay.io/fedora/fedora-bootc:<N>` where N is the current Fedora release; bumped once per Fedora cycle.
- Installs packages in §5, applies overlays in `overlays/etc`, `overlays/usr`.
- Removes: `fedora-countme`-related timers, `abrt*`, `PackageKit`, any desktop session, `cockpit`, `rsyslog`.
- Sets kernel args: `quiet loglevel=3 rd.systemd.show_status=false splash` (Tacet boot splash via plymouth theme), `mitigations=auto`.
- Creates user `tv` (uid 1000, no password, autologin), user `tacet-cec`, group `tacet`.
- Enables units: `tacet-session`, `tacet-cec`, `tacet-dns`, `firewalld`, `NetworkManager`, `chronyd`. Disables everything else non-essential.
- Result: `ghcr.io/tacet-os/tacet:<tag>` (see §7).

### 4.2 tacet-session

systemd user service for `tv`. Launches `gamescope` with the active shell as the child, restarts on crash, re-detects display mode on HDMI hotplug. Reads `/etc/tacet/session.conf` for shell choice (`kodi` default), resolution override, HDR on/off, refresh rate. ~1 shell script + unit for v1; a small C or Python supervisor later if crash-loop logic needs to be smarter.

### 4.3 tacet-cec

CEC → uinput bridge. **Fully specified in `tacet-cec-bridge-spec.md`.** First novel component built.

### 4.4 tacet-setup (first-boot wizard)

Runs as the shell on first boot (or when `/var/lib/tacet/setup-done` is absent). Remote-navigable. Steps: language → wifi/ethernet → timezone → DNS filtering on/off → update channel → done. Writes NetworkManager connection, `/etc/localtime`, `/etc/tacet/*.conf`. Written in Qt Quick (Kirigami not required); ships its own on-screen keyboard component. No network until the user configures it.

### 4.5 tacet-settings

Same codebase as tacet-setup, opened from the shell. Sections: Display (resolution/HDR/overscan), Audio (output device/passthrough), Network, DNS filter (on/off + custom blocklist URL), Updates (channel, check now, rollback), About (version, SBOM link, endpoint list), Reset (wipe `/var`, keep image).

### 4.6 tacet-update

Thin wrapper over `bootc`. Checks for a new image on a user-chosen schedule (never / daily / weekly; default **never** until the user opts in during setup). On availability: writes a flag the shell/settings shows as "Update ready — restart to apply." Applies via `bootc upgrade`; reboot; bootc rollback on boot failure (boot-count via `greenboot`-style health check: session started + CEC device present = healthy). Logs to journal only.

### 4.7 tacet-dns

Local DNS resolver with filtering. `blocky` configured with a shipped blocklist snapshot (Hagezi-style, bundled in image so first boot needs no download) and optional user-added lists. NetworkManager forced to `127.0.0.1`. Upstream resolver configurable; default is the network's DHCP-provided DNS via DoT if the upstream supports it, else plain. Off switch in settings — off means NetworkManager uses DHCP DNS directly, no proxy.

### 4.8 Firewall policy

`firewalld` zone `tacet`: inbound deny-all except mDNS (for Jellyfin discovery, toggleable) and SSH (off by default, enable in settings). Outbound: allow-all for now, with `tacet-dns` logging every resolved domain to a local ring buffer viewable in Settings → About → Network activity. Hard outbound allowlisting deferred to v2 — it breaks too many apps to ship blind.

### 4.9 Firefox TV profile

System-wide `policies.json`: telemetry off, studies off, Pocket off, sync off, DoH off (system DNS handles it), extensions preinstalled and locked: a d-pad/spatial navigation extension and uBlock Origin. Homepage = Tacet's local landing page with YouTube, Jellyfin web, and a URL bar. Launched via gamescope as a nested app from the shell. Widevine CDM install is a user-initiated action with a plain-language explanation of what it downloads and from where.

### 4.10 Shell

**v1: Kodi**, preconfigured: Estuary skin, Jellyfin add-on preinstalled, add-on repositories disabled by default, "Tacet" launcher entries for Firefox and Settings via `System.Exec`. Kodi's own update/notification network calls disabled in `advancedsettings.xml`.

**v2: tacet-shell** — a purpose-built tile launcher (Qt Quick), same toolkit as setup/settings so they share components. Out of scope for this spec; tracked separately.

### 4.11 Flatpak (optional apps)

Flatpak installed, Flathub remote **not** added by default. A `tacet` remote pointing at a curated, mirrored set (Jellyfin Media Player, SmartTube-equivalent if one exists for Linux, Moonlight) hosted on GitHub Pages. Settings → Apps lists only these. Users can add Flathub themselves with a warning.

## 5. Package list

See `PACKAGES.md` in-repo (generated from the Containerfile) for the authoritative list. Categories: base/boot, gamescope, libcec, mesa + intel-media-driver, pipewire, kodi + inputstream-adaptive, mpv, firefox, flatpak, NetworkManager, firewalld, blocky, chrony, plymouth. Removed: countme, abrt, PackageKit, cockpit, rsyslog, desktop sessions.

## 6. Privacy contract

Published as `PRIVACY.md` and shown in Settings → About. Every outbound endpoint the image can contact, by component, with the user action that triggers it:

| Endpoint | Component | Trigger | Default |
|---|---|---|---|
| `ghcr.io` | tacet-update | Update check | Off until opted in |
| Fedora mirrors | — | Never (no dnf at runtime) | Never |
| NTP pool (`pool.ntp.org` or user's) | chrony | Boot / periodic | On (time is required for TLS) |
| User's DNS upstream | tacet-dns | Any resolution | On |
| Blocklist URL | tacet-dns | Weekly refresh | Off until opted in |
| Flatpak `tacet` remote | flatpak | User installs an app | User action |
| Widevine CDM (Google) | firefox | User enables DRM | User action |
| Any site | firefox / kodi add-ons | User browses | User action |

NetworkManager connectivity check: **disabled**. Captive portals are handled by a manual "open portal page" button in Network settings.

**Rule for contributors:** any PR adding a network call must add a row to this table or it is rejected.

## 7. Versioning and release

### 7.1 Scheme

```
tacet-<FEDORA>.<MAJOR>.<PATCH>
```

- **FEDORA** — the Fedora bootc base release (43, 44, …). Bumps once per Fedora cycle. Makes the base immediately visible in bug reports and makes an EOL base obvious at a glance.
- **MAJOR** — Tacet's feature counter. Increments on user-visible features or platform changes (new shell, new hardware target, config format change). **Does not reset on a Fedora bump** — this is what users mean by "I'm on Tacet 2."
- **PATCH** — fixes and security rebuilds only; no behaviour change. Resets to 0 on any FEDORA or MAJOR bump.

Examples: `43.2.5` → Fedora bump → `44.2.0` → new feature → `44.3.0` → fix → `44.3.1`.

There is no separate MINOR; feature changes and platform changes share MAJOR. Distinguish them in the changelog, not the number.

### 7.2 Image tags

Every build pushes to `ghcr.io/tacet-os/tacet` with tags:

```
44.3.1                   ; exact version — immutable
44.3                     ; latest patch of that major on that base — moves
44                       ; latest on that Fedora base — moves
stable                   ; latest stable release — moves
testing                  ; latest pre-release — moves
44.3.1-20260913          ; full provenance: version + build date — immutable
```

Boxes subscribe to a **channel** (`stable` or `testing`), chosen at setup. `bootc switch` handles moving. Rollback is to the previous image regardless of tag.

Container images are published as a multi-arch manifest list (`linux/amd64`, `linux/arm64`; `linux/riscv64` reserved) so one tag serves every supported CPU.

Flashable images attached to GitHub Releases (per target, first install only):

```
tacet-44.3.1-x86_64.img.xz
tacet-44.3.1-rpi5.img.xz
```

Checksums + cosign signature alongside. Variant suffixes (`-vanilla`, etc.) are not needed — there is one variant.

### 7.3 Git

- Branch `main` = next MAJOR. Tags `v44.3.1`.
- Release branches `release/44.3` only if a patch is needed after `main` has moved on.
- Conventional commits; changelog generated.
- Every component daemon has its own semver inside its subdirectory (`tacet-cec 0.3.1`), pinned by the Containerfile. OS version is what users see; component versions are for contributors.

### 7.4 Security rebuilds

Fedora ships fixes continuously. Tacet rebuilds the image **weekly** on a schedule (PATCH bump only if a changed package is in the image's SBOM diff; otherwise tag moves but version doesn't). Users on `stable` with updates enabled get them; users with updates off get nothing, by design, and Settings shows "last updated N days ago" in amber past 30 days.

## 8. Build and CI

- **Build:** `podman build` → `bootc-image-builder` → `.img`. Runs on free GitHub Actions in ~10–15 min. No self-hosted runner needed.
- **Reproducibility:** Containerfile pins base image by digest; package versions locked via an `rpm-lockfile` regenerated on the weekly job. Two builds of the same commit + lockfile must produce identical `rpm -qa` output and identical file trees (timestamps normalised). Byte-identical images are a stretch goal, not a v1 claim.
- **SBOM:** `syft` output attached to every release and embedded at `/usr/share/tacet/sbom.json`.
- **Signing:** `cosign` keyless (Sigstore) on images; GPG-signed checksums on `.img` releases. bootc configured to verify signatures before switching.
- **Tests in CI:** each daemon's `make test`; Containerfile lint; a boot test in QEMU (x86) that asserts the session unit reaches `active` and `tacet-cec --list-adapters` runs without crashing (no adapter present is fine).
- **Release job:** on tag push — build, test, sign, push tags, attach images, generate changelog, update `PACKAGES.md`.

## 9. Repository layout

```
tacet/
  Containerfile
  image-builder.toml
  overlays/
    etc/            ; systemd units, NetworkManager, firewalld zone, tacet/*.conf defaults
    usr/            ; plymouth theme, policies.json, kodi advancedsettings, landing page
  components/
    tacet-cec/      ; C, own Makefile + spec
    tacet-session/
    tacet-setup/    ; Qt Quick (shared with settings)
    tacet-update/
    tacet-dns/      ; blocky config + bundled blocklist snapshot
  packaging/        ; specs for each component → built into a local repo during image build
  ci/               ; workflows, lockfile tooling, QEMU boot test
  docs/
    PRIVACY.md
    PACKAGES.md     ; generated
    HARDWARE.md
    FLASHING.md
    CONTRIBUTING.md
  CHANGELOG.md
  VERSION
```

## 10. Milestones

| Milestone | Contents | Done when |
|---|---|---|
| **M0 — Boots** | Containerfile, gamescope + Kodi session, autologin, N100 | Boots to Kodi on N100 from a flashed image |
| **M1 — Remote works** | tacet-cec | TV remote navigates Kodi end to end |
| **M2 — Updates** | GHCR push, tacet-update, rollback health check, cosign | Box updates itself from `stable` and rolls back a deliberately broken image |
| **M3 — Silent** | tacet-dns, firewall zone, telemetry stripping, PRIVACY.md, endpoint audit with `tcpdump` over 24h idle | Zero packets to anything not in the table |
| **M4 — Usable** | tacet-setup, tacet-settings, Firefox TV profile, SBOM, docs | A non-technical person can set it up with only the TV remote |
| **M5 — 1.0** | Pi 5 target, curated Flatpak remote, reproducibility report | Public release |
| **v2** | tacet-shell | — |

M0+M1 is a weekend. M2–M3 is a couple of weeks of evenings. M4 is the bulk of the work.

## 11. Open decisions (need Ricky, not Claude Code)

1. Name for the GitHub org and whether `tacet.org` / `tacetos.org` was secured.
2. Blocklist source and licence — Hagezi is permissive; confirm before bundling.
3. Whether SSH is present in the image at all (hardening) or just disabled.
4. Default NTP pool — Fedora's or a neutral one; either way it's a documented endpoint.
5. Qt Quick vs something lighter for setup/settings. Qt is the pragmatic choice for remote-navigable UI; the cost is a ~100MB image bump.

## 12. Handoff instructions for Claude Code

- Work milestone by milestone; do not start M2 before M1 is demonstrably done on hardware.
- Each component is a standalone subproject with its own README, tests, and RPM. Don't cross-import.
- Any new network endpoint → row in `PRIVACY.md` in the same PR.
- Prefer config over code. If a systemd option or NetworkManager setting solves it, no daemon.
- When a decision isn't covered here, pick the more conservative privacy option and leave a `DECISION:` comment for review.
- **Git:** commit directly to `main`; no feature branches unless explicitly asked. Small conventional commits after each working step. Never force-push or rewrite `main`. CI is the gate, not branches. Put this in `CLAUDE.md` at repo root.
