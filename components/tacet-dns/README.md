# tacet-dns

`blocky` with a bundled blocklist snapshot (spec §4.7). Milestone M3.
**Not started.** Blocklist source and licence is open decision #2.

`blocky` is not packaged in Fedora 44 or RPM Fusion (verified by the first
CI build), so this component has to bring it: a pinned upstream release
binary with a sha256 in the Containerfile, or a COPR. Either is a build-time
endpoint for `docs/PRIVACY.md`.

Already in place for it: NetworkManager's connectivity check is off and the
firewalld `tacet` zone is the default.
