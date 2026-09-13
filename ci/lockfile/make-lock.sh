#!/bin/bash
# Record the exact package set of a built image as ci/lockfile/packages.lock.
#
#   make-lock.sh <image-ref> [output]
#
# Run by the weekly job after an unpinned build. The output is committed; the
# next builds pin to it (install-packages.sh).
set -euo pipefail

image=$1
out=${2:-ci/lockfile/packages.lock}

podman run --rm "$image" rpm -qa --qf '%{NAME}\t%{EVR}\t%{ARCH}\n' \
  | LC_ALL=C sort > "$out.tmp"
mv "$out.tmp" "$out"
echo "wrote $out ($(wc -l < "$out") packages)"
