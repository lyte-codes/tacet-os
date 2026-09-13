#!/bin/bash
# Resolve the moving base tag to an immutable digest reference.
#
#   resolve-base-digest.sh [tag-ref] > ci/lockfile/base-digest
#
# Needs skopeo. The weekly job commits the result; ci/build.sh passes it to
# `podman build --build-arg BASE_IMAGE=...`.
set -euo pipefail

ref=${1:-quay.io/fedora/fedora-bootc:44}
digest=$(skopeo inspect --raw "docker://$ref" | sha256sum | cut -d' ' -f1)
printf '%s@sha256:%s\n' "${ref%%@*}" "$digest"
