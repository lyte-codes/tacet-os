#!/bin/bash
# Turn a built container image into a raw disk with bootc-image-builder.
#
#   ci/make-disk.sh [image-ref] [output-dir] [type]
#
# Must run as root (or via sudo) because bootc-image-builder is privileged and
# reads root's container storage. Output: <output-dir>/image/disk.raw for
# --type raw.
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
image=${1:-localhost/tacet:dev}
out=${2:-$root/output}
type=${3:-raw}
BIB=${BIB:-quay.io/centos-bootc/bootc-image-builder:latest}

mkdir -p "$out"
podman run --rm --privileged --pull=newer \
  --security-opt label=type:unconfined_t \
  -v "$root/image-builder.toml:/config.toml:ro" \
  -v "$out:/output" \
  -v /var/lib/containers/storage:/var/lib/containers/storage \
  "$BIB" \
  --type "$type" --rootfs ext4 --config /config.toml --local "$image"

ls -la "$out"/"$type"/ "$out"/image/ 2>/dev/null || true
