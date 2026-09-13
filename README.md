# Tacet OS

A privacy-first TV operating system. Flash it to a small box, plug it into a
dumb (or dumbed-down) TV, and nothing phones home.

- **Silent by default.** No telemetry, no crash reporting, no connectivity
  probes, no update checks without consent. Every endpoint the image can
  contact is listed in [`docs/PRIVACY.md`](docs/PRIVACY.md).
- **Appliance reliability.** Fedora bootc: atomic image updates with automatic
  rollback. A power cut cannot brick it.
- **TV-native input.** The TV's own remote drives everything over HDMI-CEC.
- **Auditable.** Reproducible builds, published SBOM, signed images.

The full design is in [`docs/tacet-os-spec.md`](docs/tacet-os-spec.md).
Current progress against the milestones is in [`docs/STATUS.md`](docs/STATUS.md).

## Layout

```
Containerfile          bootc image: Fedora bootc 44 + packages + overlays + Tacet RPMs
image-builder.toml     bootc-image-builder config for flashable .img files
overlays/etc, usr      config dropped verbatim into the image
components/            Tacet daemons, one subproject each (README, VERSION, Makefile, tests)
packaging/             RPM specs + the script that builds them during the image build
ci/                    lockfile tooling, PACKAGES.md generator, QEMU boot test
.github/workflows/     CI (build, test, release, weekly rebuild)
docs/                  spec, privacy contract, hardware, flashing, contributing
```

## Building

```
make lint && make test        # no podman needed
make build                    # podman build -t localhost/tacet:dev .
make disk                     # bootc-image-builder -> output/disk.raw (needs root/podman)
make boot-test                # boots output/disk.raw in QEMU and checks the session comes up
```

See [`docs/FLASHING.md`](docs/FLASHING.md) to put an image on a box and
[`docs/CONTRIBUTING.md`](docs/CONTRIBUTING.md) before opening a PR.
