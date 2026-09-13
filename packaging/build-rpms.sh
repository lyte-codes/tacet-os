#!/bin/bash
# Build every component that has a spec in packaging/ into RPMs.
#
#   build-rpms.sh <repo-root> <output-dir>
#
# For each packaging/<name>.spec there must be a components/<name>/ directory
# with a VERSION file. The directory is tarred reproducibly as
# <name>-<version>.tar.gz and built with `tacet_version` defined.
set -euo pipefail

src=${1:-.}
out=${2:-out}
top=$(mktemp -d)
mkdir -p "$top"/{SOURCES,SPECS,BUILD,RPMS,SRPMS} "$out"

shopt -s nullglob
specs=("$src"/packaging/*.spec)
if [ ${#specs[@]} -eq 0 ]; then
  echo "build-rpms: no specs in $src/packaging" >&2
  exit 1
fi

for spec in "${specs[@]}"; do
  name=$(basename "$spec" .spec)
  dir="$src/components/$name"
  if [ ! -f "$dir/VERSION" ]; then
    echo "build-rpms: $spec has no components/$name/VERSION" >&2
    exit 1
  fi
  version=$(tr -d '[:space:]' < "$dir/VERSION")
  echo "build-rpms: $name $version"
  tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner \
      --transform "s,^${name}/,${name}-${version}/," \
      -C "$src/components" -czf "$top/SOURCES/${name}-${version}.tar.gz" "$name"
  rpmbuild --define "_topdir $top" --define "tacet_version $version" \
           --define "source_date_epoch_from_changelog 0" \
           -bb "$spec"
done

find "$top/RPMS" -name '*.rpm' -exec cp -v {} "$out"/ \;
rm -rf "$top"
