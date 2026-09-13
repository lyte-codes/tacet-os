#!/usr/bin/env python3
"""Generate docs/PACKAGES.md from the package block in the Containerfile.

The block starts at the line containing `install-packages.sh` inside a RUN and
ends at the first line without a trailing backslash. Inside it, lines of the
form `# category: <name>` open a category and every other non-empty line is
one package. Local RPM globs (`/tmp/...*.rpm`) are the Tacet components and
are listed by name from components/*/VERSION.

    ci/gen-packages-md.py [Containerfile] > docs/PACKAGES.md
    ci/gen-packages-md.py --check          # exit 1 if docs/PACKAGES.md is stale
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MARKER = "install-packages.sh"


def parse_block(text):
    lines = text.splitlines()
    start = next((i for i, l in enumerate(lines) if l.lstrip().startswith("RUN") and MARKER in l), None)
    if start is None:
        raise SystemExit(f"gen-packages-md: no RUN line containing {MARKER}")
    categories = []
    current = None
    i = start
    while True:
        line = lines[i]
        i += 1
        stripped = line.strip()
        if stripped.startswith("#"):
            # Comment lines are stripped by the Dockerfile parser before the
            # shell sees them, so they neither continue nor end the block.
            if stripped.startswith("# category:"):
                current = (stripped.split(":", 1)[1].strip(), [])
                categories.append(current)
            continue
        continues = stripped.endswith("\\")
        body = stripped[:-1].strip() if continues else stripped
        if body and MARKER not in body:
            if current is None:
                raise SystemExit(f"gen-packages-md: package before any category: {body}")
            current[1].append(body)
        if not continues:
            break
    return categories


def component_versions():
    out = []
    for vf in sorted((ROOT / "components").glob("*/VERSION")):
        out.append((vf.parent.name, vf.read_text().strip()))
    return out


def base_image(text):
    m = re.search(r"^ARG BASE_IMAGE=(\S+)", text, re.M)
    return m.group(1) if m else "unknown"


def render(text):
    cats = parse_block(text)
    comps = component_versions()
    lines = [
        "# Packages",
        "",
        "Generated from the `Containerfile` by `ci/gen-packages-md.py`; do not edit by hand.",
        "Run `make packages` after changing the package block.",
        "",
        f"Base image: `{base_image(text)}` (pinned by digest in CI via `ci/lockfile/base-digest`).",
        "",
        "Exact versions of everything in a built image are in `ci/lockfile/packages.lock`",
        "once the weekly job has run, and in the SBOM attached to each release.",
        "",
        "## Installed (by category)",
        "",
    ]
    for name, pkgs in cats:
        lines.append(f"### {name}")
        lines.append("")
        for p in pkgs:
            if p.endswith(".rpm"):
                for cname, cver in comps:
                    lines.append(f"- `{cname}` {cver} (built from `components/{cname}/`)")
            else:
                lines.append(f"- `{p}`")
        lines.append("")
    lines += [
        "## Removed",
        "",
        "Removed after install if the base image carries them: `abrt*`, `PackageKit*`,",
        "`cockpit*`, `rsyslog*`, `gnome-*`, `plasma-*`, `sddm*`, `gdm*`, `xorg-x11-server-Xorg`.",
        "",
        "Fedora `countme` is disabled in every repo file and in `dnf.conf`; the",
        "`rpm-ostree-countme`, `dnf-makecache` and `bootc-fetch-apply-updates` timers are masked.",
        "",
    ]
    return "\n".join(lines)


def main(argv):
    check = "--check" in argv
    args = [a for a in argv if not a.startswith("--")]
    cf = Path(args[0]) if args else ROOT / "Containerfile"
    rendered = render(cf.read_text())
    if check:
        target = ROOT / "docs" / "PACKAGES.md"
        if not target.exists() or target.read_text() != rendered:
            print("docs/PACKAGES.md is stale; run `make packages`", file=sys.stderr)
            return 1
        return 0
    sys.stdout.write(rendered)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
