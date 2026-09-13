# Tacet OS — instructions for Claude Code

Tacet is a privacy-first TV operating system built as a Fedora bootc
container image. The full system specification is `docs/tacet-os-spec.md`;
read it before touching anything. This file only holds the working rules.

## Git

- Commit directly to `main`. No feature branches unless explicitly asked.
- Small conventional commits (`feat:`, `fix:`, `chore:`, `docs:`, `ci:`) after
  each working step. The changelog is generated from them.
- Never force-push or rewrite `main`. CI is the gate, not branches.
- Tags are `v<FEDORA>.<MAJOR>.<PATCH>` (see spec §7). `VERSION` holds the
  current version; component versions live in `components/*/VERSION`.

## How to work

- Milestone by milestone (spec §10). Do not start M2 before M1 is
  demonstrably done on hardware. Current milestone: see `docs/STATUS.md`.
- Each component under `components/` is a standalone subproject with its own
  `README.md`, `VERSION`, `Makefile` (`make test`, `make install DESTDIR=`)
  and an RPM spec in `packaging/`. Components never import from each other.
- Any new outbound network endpoint requires a row in `docs/PRIVACY.md` in the
  same commit. No row, no merge.
- Prefer config over code. If a systemd option or NetworkManager setting
  solves it, do not write a daemon.
- When a decision is not covered by the spec, pick the more conservative
  privacy option and leave a `DECISION:` comment next to it for review.
  `grep -rn 'DECISION:'` lists every one.
- Package names in the `Containerfile` are only proven by a CI build. When
  adding packages, keep the `# category:` annotations; `docs/PACKAGES.md` is
  generated from them (`make packages`) and CI fails if it is stale.

## Local checks

```
make lint      # shellcheck, hadolint (if installed), python syntax, unit files
make test      # every component's `make test`
make packages  # regenerate docs/PACKAGES.md from the Containerfile
make build     # podman build of the bootc image (needs podman)
```
