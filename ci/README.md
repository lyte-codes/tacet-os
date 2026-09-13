# ci

Scripts used by the workflows in `.github/workflows/` and by `make`. All of
them run locally too.

| Script | Purpose |
|---|---|
| `build.sh` | `podman build` of the bootc image, pinned to `lockfile/base-digest` when present |
| `make-disk.sh` | bootc-image-builder → raw disk (`output/image/disk.raw`); needs root |
| `boot-test.sh` | boots a raw disk in QEMU/OVMF, waits for `TACET-BOOT-REPORT`, asserts the session is active |
| `gen-packages-md.py` | renders `docs/PACKAGES.md` from the Containerfile's package block (`--check` for CI) |
| `version.py` | version parsing, PATCH/MAJOR/FEDORA bumps, image tag lists per spec §7.2 |
| `lockfile/` | base digest + package lock tooling (see its README) |

Workflows live in `.github/workflows/` because GitHub requires that path; the
spec's `ci/` directory holds everything else.
