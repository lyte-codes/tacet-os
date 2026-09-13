# Privacy contract

Tacet is silent by default: no outbound traffic the user did not cause. This
document is the complete list of endpoints the image can contact, by
component, with the user action that triggers each one. It is shown in
Settings → About and enforced in review: **any change that adds a network
call must add a row here in the same commit, or it is rejected.**

## Runtime endpoints

| Endpoint | Component | Trigger | Default |
|---|---|---|---|
| `ghcr.io` | tacet-update | Update check | Off until opted in |
| Fedora mirrors | — | Never (no dnf at runtime) | Never |
| NTP pool (`2.fedora.pool.ntp.org` or user's) | chrony | Boot / periodic | On (time is required for TLS) |
| User's DNS upstream | tacet-dns | Any resolution | On |
| Blocklist URL | tacet-dns | Weekly refresh | Off until opted in |
| Flatpak `tacet` remote | flatpak | User installs an app | User action |
| Widevine CDM (Google) | firefox | User enables DRM | User action |
| Any site | firefox / kodi add-ons | User browses | User action |

NetworkManager connectivity check: **disabled**
(`overlays/etc/NetworkManager/conf.d/10-tacet.conf`). Captive portals are
handled by a manual "open portal page" button in Network settings.

## What the M0 image actually does today

Components that do not exist yet cannot phone home, so the effective list for
the current image is shorter than the contract:

| Endpoint | Status in M0 image |
|---|---|
| NTP pool | Active. chrony uses Fedora's default `2.fedora.pool.ntp.org` until open decision #4 picks a pool. |
| DHCP-provided DNS | Active, directly (tacet-dns lands in M3). |
| `ghcr.io` | Nothing contacts it. `bootc-fetch-apply-updates.timer` is masked. |
| Kodi | Official add-on repository and version checker are disabled in `addon-manifest.xml`; add-on update checks, RSS feed, zeroconf, UPnP, web server and AirPlay are off in the seeded `guisettings.xml`. Jellyfin add-on not yet bundled. |
| Firefox | Installed but not launched by anything yet; the TV profile with telemetry off lands in M4. Do not treat Firefox as silent until then. |
| tacet-cec | No network at all: the unit runs with `PrivateNetwork=yes`, an empty capability set and a closed device policy. The only thing it transmits is the CEC "Active Source" message on the HDMI wire. |
| Fedora countme | Disabled in `dnf.conf` and every `.repo` file; `rpm-ostree-countme` timer masked. |
| mDNS | Inbound allowed by the firewall zone; Kodi's own zeroconf announcement is off. Nothing announces. |
| DHCP | Hostname is never sent (`hostname-mode=none`, `dhcp-send-hostname=false`); wifi MAC randomised per network. |

## Build-time endpoints (not in the image)

These are contacted by CI while building, never by a running box:

| Endpoint | Purpose |
|---|---|
| `quay.io/fedora/fedora-bootc` | Base image |
| Fedora mirrors | Package install during `podman build` |
| RPM Fusion mirrors | Kodi, intel-media-driver |
| `ghcr.io` | Publishing the built image |
| Sigstore (`fulcio`, `rekor`) | Keyless signing at release |

## Audit

M3 closes with a 24-hour idle `tcpdump` on a box in its default state; the
capture must contain nothing outside the runtime table.
