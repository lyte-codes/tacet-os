#!/bin/bash
# Install packages, pinning each name to the NEVRA in the lockfile if present.
#
#   install-packages.sh <lockfile> <name-or-rpm-path>...
#
# Lockfile format (ci/lockfile/make-lock.sh): one `NAME<TAB>EVR<TAB>ARCH` per
# line, produced from `rpm -qa` of a finished image. A name missing from the
# lockfile installs unpinned; a pinned NEVRA that no longer exists on the
# mirrors fails the build, which is what the weekly relock job is for.
set -euo pipefail

lock=$1
shift

resolve() {
  local name=$1
  case "$name" in
    /*|*.rpm) printf '%s\n' "$name"; return ;;
  esac
  if [ -s "$lock" ]; then
    local hit
    hit=$(awk -F'\t' -v n="$name" '$1 == n { print $1 "-" $2 "." $3; exit }' "$lock")
    if [ -n "$hit" ]; then printf '%s\n' "$hit"; return; fi
  fi
  printf '%s\n' "$name"
}

specs=()
for name in "$@"; do
  specs+=("$(resolve "$name")")
done

if [ -s "$lock" ]; then
  echo "install-packages: lockfile $lock has $(wc -l < "$lock") entries"
else
  echo "install-packages: no lockfile, installing unpinned"
fi

exec dnf -y install --setopt=install_weak_deps=False "${specs[@]}"
