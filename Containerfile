# Tacet OS image. See docs/tacet-os-spec.md §4.1.
#
# Stage 1 builds the Tacet component RPMs from components/ + packaging/.
# Stage 2 layers packages, overlays and those RPMs on top of Fedora bootc.
#
# BASE_IMAGE is pinned by digest in CI (ci/lockfile/base-digest, resolved by
# the weekly job). Locally it falls back to the moving tag.
ARG BASE_IMAGE=quay.io/fedora/fedora-bootc:44
ARG BUILDER_IMAGE=quay.io/fedora/fedora:44

# ---------------------------------------------------------------------------
FROM ${BUILDER_IMAGE} AS rpmbuild
RUN dnf -y install rpm-build make tar systemd-rpm-macros && dnf clean all
COPY VERSION /src/VERSION
COPY components/ /src/components/
COPY packaging/ /src/packaging/
RUN /src/packaging/build-rpms.sh /src /out

# ---------------------------------------------------------------------------
FROM ${BASE_IMAGE}
ARG FEDORA_RELEASE=44

COPY ci/lockfile/ /tmp/lockfile/
COPY --from=rpmbuild /out/ /tmp/tacet-rpms/

# Build-time only repositories. Kodi and intel-media-driver come from RPM
# Fusion. The .repo files stay in the image but nothing uses dnf at runtime
# (docs/PRIVACY.md: "Fedora mirrors — never").
RUN dnf -y install \
      "https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-${FEDORA_RELEASE}.noarch.rpm" \
      "https://mirrors.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-${FEDORA_RELEASE}.noarch.rpm"

# --- packages ---------------------------------------------------------------
# One package per line, grouped under `# category: <name>` comments.
# docs/PACKAGES.md is generated from this block by ci/gen-packages-md.py;
# `make packages` regenerates it and CI fails if it is stale.
# ci/lockfile/install-packages.sh pins each name to the NEVRA recorded in
# ci/lockfile/packages.lock when that file exists (weekly job), otherwise it
# installs the latest available.
RUN /tmp/lockfile/install-packages.sh /tmp/lockfile/packages.lock \
    # category: base/boot
    linux-firmware \
    plymouth \
    plymouth-plugin-script \
    plymouth-scripts \
    # category: gamescope
    gamescope \
    xorg-x11-server-Xwayland \
    vulkan-loader \
    mesa-vulkan-drivers \
    libinput \
    # category: libcec
    libcec \
    v4l-utils \
    # category: mesa + intel-media-driver
    mesa-dri-drivers \
    mesa-va-drivers \
    intel-media-driver \
    libva \
    libva-utils \
    # category: pipewire
    pipewire \
    pipewire-alsa \
    pipewire-pulseaudio \
    wireplumber \
    alsa-utils \
    # category: kodi + inputstream-adaptive
    kodi \
    kodi-inputstream-adaptive \
    # category: mpv
    mpv \
    # category: firefox
    firefox \
    # category: flatpak
    flatpak \
    # category: NetworkManager
    NetworkManager \
    NetworkManager-wifi \
    iw \
    # category: firewalld
    firewalld \
    # category: chrony
    chrony \
    # category: fonts
    google-noto-sans-fonts \
    google-noto-sans-cjk-vf-fonts \
    google-noto-emoji-fonts \
    # category: tacet
    /tmp/tacet-rpms/*.rpm
# --- end packages -----------------------------------------------------------

# Remove what must not be on a silent appliance (spec §4.1). Each pattern is
# only removed if present so the step survives base-image changes.
# hadolint ignore=SC2086
RUN set -eu; \
    pkgs="$(rpm -qa 'abrt*' 'PackageKit*' 'cockpit*' 'rsyslog*' 'gnome-*' 'plasma-*' 'sddm*' 'gdm*' 'xorg-x11-server-Xorg' || true)"; \
    if [ -n "$pkgs" ]; then dnf -y remove $pkgs; fi; \
    dnf clean all; \
    rm -rf /var/cache/dnf /var/cache/libdnf5 /var/lib/dnf /tmp/lockfile /tmp/tacet-rpms

# Fedora "countme" telemetry: off in every repo file and in dnf itself, and the
# rpm-ostree/bootc timers that would report it are masked below.
RUN set -eu; \
    sed -i 's/^countme=1/countme=0/' /etc/yum.repos.d/*.repo; \
    printf '\n# Tacet: never report usage to Fedora\ncountme=false\n' >> /etc/dnf/dnf.conf

COPY overlays/etc/ /etc/
COPY overlays/usr/ /usr/
COPY VERSION /usr/share/tacet/VERSION

# Users and groups (spec §4.1). sysusers.d is the source of truth; running it
# here bakes them into /etc/passwd so the image is deterministic.
# DECISION: `tv` has a locked password rather than an empty one. "No password"
# is satisfied by the kiosk unit logging it in via PAM without authentication;
# a locked password keeps password login impossible if sshd is ever enabled.
RUN systemd-sysusers

# Kodi: only add-ons listed in addon-manifest.xml are enabled on a fresh
# profile. Dropping the official repository and the version checker keeps Kodi
# from contacting mirrors.kodi.tv on its own (spec §4.10).
# DECISION: this is done at image build rather than in advancedsettings.xml
# because Kodi has no advancedsettings knob for disabling bundled add-ons.
RUN set -eu; \
    m=/usr/share/kodi/system/addon-manifest.xml; \
    sed -i -e '/repository.xbmc.org/d' -e '/service.xbmc.versioncheck/d' "$m"

# Units (spec §4.1). tacet-cec and tacet-dns are enabled when their milestones
# land (M1, M3); enabling a missing unit fails the build on purpose.
RUN set -eu; \
    systemctl enable tacet-session.service tacet-boot-report.service \
                     firewalld.service NetworkManager.service chronyd.service; \
    systemctl set-default graphical.target; \
    systemctl mask getty@tty1.service \
                   rpm-ostree-countme.timer rpm-ostree-countme.service \
                   dnf-makecache.timer dnf5-makecache.timer \
                   bootc-fetch-apply-updates.timer bootc-fetch-apply-updates.service; \
    for u in sshd.service sshd.socket dnf-automatic.timer systemd-homed.service \
             remote-fs.target nfs-client.target rpcbind.socket; do \
      systemctl disable "$u" 2>/dev/null || true; \
    done; \
    sed -i 's/^DefaultZone=.*/DefaultZone=tacet/' /etc/firewalld/firewalld.conf

# Plymouth: the theme is chosen in /etc/plymouth/plymouthd.conf (overlay); the
# initramfs must be rebuilt so the theme is inside it.
RUN set -eu; \
    kver="$(basename /usr/lib/modules/*)"; \
    dracut --no-hostonly --reproducible --force --kver "$kver" \
           --add plymouth "/usr/lib/modules/${kver}/initramfs.img"

RUN bootc container lint

LABEL org.opencontainers.image.title="Tacet OS" \
      org.opencontainers.image.description="Privacy-first TV operating system (Fedora bootc)" \
      org.opencontainers.image.source="https://github.com/lyte-codes/tacet-os" \
      org.opencontainers.image.licenses="TBD"
