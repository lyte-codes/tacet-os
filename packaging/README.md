# packaging

One RPM spec per component. `build-rpms.sh` runs in the Containerfile's first
stage (a plain Fedora container with `rpm-build`), builds every spec against
its `components/<name>/` tree, and the second stage installs the resulting
RPMs in the same `dnf install` transaction as everything else.

Conventions:

- The spec's `Version` is `%{tacet_version}`, injected from
  `components/<name>/VERSION`. Bump the component's VERSION, not the spec.
- `%install` is always `%make_install PREFIX=%{_prefix}`; the component's
  Makefile owns the file layout.
- Systemd units use the `%systemd_post`/`%systemd_preun` macros so the RPM is
  correct outside the image build too.
- Licence is `TBD` until the project licence is chosen (open decision).

Local check without podman, on a Fedora host with `rpm-build`:

```
./packaging/build-rpms.sh . /tmp/tacet-rpms && rpm -qlp /tmp/tacet-rpms/*.rpm
```
