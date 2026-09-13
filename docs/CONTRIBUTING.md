# Contributing

Read `docs/tacet-os-spec.md` first. The spec is the design; this file is the
mechanics.

## Ground rules

1. **Any new network call needs a row in `docs/PRIVACY.md`** in the same
   commit. No row, no merge. Build-time-only endpoints go in that file's
   build-time table.
2. **Prefer config over code.** A systemd option, a NetworkManager key or a
   firewalld zone beats a daemon.
3. **Components are standalone.** `components/<name>/` has its own README,
   VERSION, Makefile (`test`, `lint`, `install DESTDIR=`) and a spec in
   `packaging/`. Components never import each other.
4. **Decisions the spec does not cover** take the more private option and get
   a `DECISION:` comment for review. `grep -rn 'DECISION:'` lists them.
5. **Conventional commits** (`feat:`, `fix:`, `docs:`, `chore:`, `ci:`).
   The changelog is generated from them.

## Before pushing

```
make lint     # shellcheck, hadolint (if installed), python, systemd unit verify
make test     # component tests + ci tooling tests
make packages # if you touched the package block in the Containerfile
```

CI runs the same plus a full image build, a bootc-image-builder disk and a
QEMU boot test. Package names are only proven by that build.

## Adding a package

Add it to the annotated block in the `Containerfile` under the right
`# category:` line, run `make packages`, commit both. If the package opens a
socket on its own, see rule 1.

## Adding a component

1. `components/<name>/` with README.md, VERSION (`0.1.0`), Makefile, tests.
2. `packaging/<name>.spec` using `%{tacet_version}` and `%make_install`.
3. Enable its unit in the Containerfile's unit step.
4. `docs/STATUS.md` milestone row.

## Versioning

`VERSION` is `<FEDORA>.<MAJOR>.<PATCH>`; see spec §7. Human-driven bumps:

- feature or platform change → `python3 ci/version.py bump major $(cat VERSION)`
- new Fedora base → bump `fedora`, and update `ARG BASE_IMAGE` / `FEDORA_RELEASE`
  in the Containerfile

Then commit `VERSION` + `CHANGELOG.md`, tag `v<version>`, push the tag. The
release workflow does the rest. PATCH bumps are the weekly job's business.
