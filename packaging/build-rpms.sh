#!/bin/bash
# Build every component that has a spec in packaging/ into RPMs.
#
#   build-rpms.sh <repo-root> <output-dir>
#
# Every components/<name>/VERSION is built from packaging/<name>.spec or
# components/<name>/packaging/<name>.spec (component-local, spec §11). The directory is tarred reproducibly as
# <name>-<version>.tar.gz and built with `tacet_version` defined.
set -euo pipefail

src=${1:-.}
out=${2:-out}
top=$(mktemp -d)
mkdir -p "$top"/{SOURCES,SPECS,BUILD,RPMS,SRPMS} "$out"

shopt -s nullglob
built=0
for vfile in "$src"/components/*/VERSION; do
  dir=$(dirname "$vfile")
  name=$(basename "$dir")
  spec=""
  for cand in "$src/packaging/$name.spec" "$dir/packaging/$name.spec"; do
    [ -f "$cand" ] && spec=$cand && break
  done
  if [ -z "$spec" ]; then
    echo "build-rpms: $name has a VERSION but no spec, skipping" >&2
    continue
  fi
  version=$(tr -d '[:space:]' < "$vfile")
  echo "build-rpms: $name $version ($spec)"
  tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner \
      --transform "s,^${name}/,${name}-${version}/," \
      --exclude='*.o' --exclude='run-tests' --exclude="${name}/${name}" \
      -C "$src/components" -czf "$top/SOURCES/${name}-${version}.tar.gz" "$name"
  rpmbuild --define "_topdir $top" --define "tacet_version $version" \
           --define "source_date_epoch_from_changelog 0" \
           -bb "$spec"
  built=$((built + 1))
done
if [ "$built" -eq 0 ]; then
  echo "build-rpms: nothing built" >&2
  exit 1
fi

find "$top/RPMS" -name '*.rpm' -exec cp -v {} "$out"/ \;
rm -rf "$top"
