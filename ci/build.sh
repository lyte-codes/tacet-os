#!/bin/bash
# Build the Tacet bootc container image.
#
#   ci/build.sh [image-ref]        default localhost/tacet:dev
#
# Uses ci/lockfile/base-digest for the base image when present so CI builds
# are pinned; local builds without it use the moving tag in the Containerfile.
# Set PODMAN to `sudo podman` when the image must land in root's storage
# (bootc-image-builder needs that).
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
image=${1:-localhost/tacet:dev}
PODMAN=${PODMAN:-podman}

args=()
if [ -s "$root/ci/lockfile/base-digest" ]; then
  base=$(tr -d '[:space:]' < "$root/ci/lockfile/base-digest")
  echo "build: base image pinned to $base"
  args+=(--build-arg "BASE_IMAGE=$base")
fi

version=$(tr -d '[:space:]' < "$root/VERSION")
exec $PODMAN build \
  "${args[@]}" \
  --label "org.opencontainers.image.version=$version" \
  --label "org.opencontainers.image.revision=$(git -C "$root" rev-parse HEAD 2>/dev/null || echo unknown)" \
  -t "$image" -f "$root/Containerfile" "$root"
