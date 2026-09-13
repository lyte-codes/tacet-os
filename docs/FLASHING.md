# Flashing

Releases attach one flashable image per target:

```
tacet-<version>-x86_64.img.xz     Intel N100 and other UEFI x86_64 boxes
tacet-<version>-rpi5.img.xz       Raspberry Pi 5 (when M5 lands)
SHA256SUMS                        checksums
SHA256SUMS.sigstore.json          cosign keyless signature bundle for SHA256SUMS
SHA256SUMS.asc                    GPG signature (when a release key is configured)
```

## Verify

```
sha256sum -c SHA256SUMS
cosign verify-blob --bundle SHA256SUMS.sigstore.json \
  --certificate-identity-regexp 'https://github.com/.*/tacet-os/.github/workflows/release.yml@.*' \
  --certificate-oidc-issuer https://token.actions.githubusercontent.com \
  SHA256SUMS
```

## Write to the box's disk

The image is a raw GPT disk with an EFI system partition and an ext4 root.
Write it to the box's internal drive from a live USB, or to a USB stick and
boot from that:

```
xz -dc tacet-<version>-x86_64.img.xz | sudo dd of=/dev/sdX bs=4M status=progress conv=fsync
```

Replace `/dev/sdX` with the target device. Everything on it is erased.

The root partition is sized at build time (10 GiB); the remaining space is
not used yet. Growing `/var` onto it is on the M2 list.

## First boot

Boot with the TV connected over HDMI. The Tacet splash appears, then Kodi.
There is no login prompt and no desktop; the session is the `tv` user on
tty1. Until M4 ships the setup wizard, network is configured from Kodi's
settings or by editing `/etc/NetworkManager/system-connections/` from a
keyboard.

## Updating an installed box to a newer image

Not needed for M0. Once M2 lands, `tacet-update` wraps `bootc upgrade`;
until then `sudo bootc switch ghcr.io/<owner>/tacet:stable` from a root
shell does the same thing without a health check.
