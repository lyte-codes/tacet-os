#!/usr/bin/env python3
"""Version and image-tag helper for the spec §7 scheme <FEDORA>.<MAJOR>.<PATCH>.

    ci/version.py tags 44.3.1 --channel stable --date 20260913
        44.3.1 44.3 44 stable 44.3.1-20260913
    ci/version.py tags 44.3.0-rc1 --channel testing --date 20260913
        44.3.0-rc1 testing 44.3.0-rc1-20260913
    ci/version.py bump patch 44.3.1      -> 44.3.2
    ci/version.py bump major 44.3.1      -> 44.4.0
    ci/version.py bump fedora 44.3.1     -> 45.3.0
    ci/version.py check-tag v44.3.1 [VERSION-file]   exit 1 on mismatch
"""
import argparse
import re
import sys
from pathlib import Path

VERSION_RE = re.compile(r"^(\d+)\.(\d+)\.(\d+)(?:-([0-9A-Za-z.]+))?$")


def parse(version):
    m = VERSION_RE.match(version.strip())
    if not m:
        raise ValueError(f"not a Tacet version: {version!r}")
    fedora, major, patch, pre = m.groups()
    return int(fedora), int(major), int(patch), pre


def is_prerelease(version):
    return parse(version)[3] is not None


def tags(version, channel, date):
    fedora, major, patch, pre = parse(version)
    if channel not in ("stable", "testing"):
        raise ValueError("channel must be stable or testing")
    if pre and channel == "stable":
        raise ValueError("a pre-release cannot be published to the stable channel")
    out = [version]
    if not pre:
        out += [f"{fedora}.{major}", f"{fedora}"]
    out.append(channel)
    if date:
        out.append(f"{version}-{date}")
    return out


def bump(part, version):
    fedora, major, patch, pre = parse(version)
    if pre:
        raise ValueError("bump a release version, not a pre-release")
    if part == "patch":
        return f"{fedora}.{major}.{patch + 1}"
    if part == "major":
        return f"{fedora}.{major + 1}.0"
    if part == "fedora":
        return f"{fedora + 1}.{major}.0"
    raise ValueError("part must be patch, major or fedora")


def main(argv):
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    t = sub.add_parser("tags")
    t.add_argument("version")
    t.add_argument("--channel", default="stable")
    t.add_argument("--date", default="")
    b = sub.add_parser("bump")
    b.add_argument("part", choices=["patch", "major", "fedora"])
    b.add_argument("version")
    c = sub.add_parser("check-tag")
    c.add_argument("tag")
    c.add_argument("version_file", nargs="?", default=str(Path(__file__).resolve().parent.parent / "VERSION"))
    pr = sub.add_parser("is-prerelease")
    pr.add_argument("version")
    a = p.parse_args(argv)
    try:
        if a.cmd == "tags":
            print(" ".join(tags(a.version, a.channel, a.date)))
        elif a.cmd == "bump":
            print(bump(a.part, a.version))
        elif a.cmd == "check-tag":
            want = Path(a.version_file).read_text().strip()
            got = a.tag[1:] if a.tag.startswith("v") else a.tag
            parse(got)
            if got != want:
                print(f"tag {a.tag} does not match VERSION {want}", file=sys.stderr)
                return 1
        elif a.cmd == "is-prerelease":
            return 0 if is_prerelease(a.version) else 1
    except ValueError as e:
        print(f"version.py: {e}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
