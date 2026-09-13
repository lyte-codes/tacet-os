# Packages

Generated from the `Containerfile` by `ci/gen-packages-md.py`; do not edit by hand.
Run `make packages` after changing the package block.

Base image: `quay.io/fedora/fedora-bootc:44` (pinned by digest in CI via `ci/lockfile/base-digest`).

Exact versions of everything in a built image are in `ci/lockfile/packages.lock`
once the weekly job has run, and in the SBOM attached to each release.

## Installed (by category)

### base/boot

- `linux-firmware`
- `plymouth`
- `plymouth-plugin-script`
- `plymouth-scripts`

### gamescope

- `gamescope`
- `xorg-x11-server-Xwayland`
- `vulkan-loader`
- `mesa-vulkan-drivers`
- `libinput`

### libcec

- `libcec`
- `v4l-utils`

### mesa + intel-media-driver

- `mesa-dri-drivers`
- `mesa-va-drivers`
- `intel-media-driver`
- `libva`
- `libva-utils`

### pipewire

- `pipewire`
- `pipewire-alsa`
- `pipewire-pulseaudio`
- `wireplumber`
- `alsa-utils`

### kodi + inputstream-adaptive

- `kodi`
- `kodi-inputstream-adaptive`

### mpv

- `mpv`

### firefox

- `firefox`

### flatpak

- `flatpak`

### NetworkManager

- `NetworkManager`
- `NetworkManager-wifi`
- `iw`

### firewalld

- `firewalld`

### chrony

- `chrony`

### fonts

- `google-noto-sans-fonts`
- `google-noto-sans-cjk-vf-fonts`
- `google-noto-emoji-fonts`

### tacet

- `tacet-session` 0.1.0 (built from `components/tacet-session/`)

## Removed

Removed after install if the base image carries them: `abrt*`, `PackageKit*`,
`cockpit*`, `rsyslog*`, `gnome-*`, `plasma-*`, `sddm*`, `gdm*`, `xorg-x11-server-Xorg`.

Fedora `countme` is disabled in every repo file and in `dnf.conf`; the
`rpm-ostree-countme`, `dnf-makecache` and `bootc-fetch-apply-updates` timers are masked.
