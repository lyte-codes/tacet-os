#!/bin/bash
# tacet-session tests. Runs the script in dry-run mode against temporary
# config files and checks the gamescope command line it would exec.
set -euo pipefail

here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
script="$here/../tacet-session"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

pass=0
fail=0

run_session() {
  # $1 = config file (may be missing), remaining = extra env assignments
  local conf=$1; shift
  env HOME="$tmp/home" TACET_SESSION_DRY_RUN=1 TACET_SESSION_CONF="$conf" \
      TACET_KODI_SEED_DIR="$tmp/seed" "$@" bash "$script" 2>"$tmp/stderr"
}

check() {
  local name=$1 haystack=$2 needle=$3
  if [[ $haystack == *"$needle"* ]]; then
    pass=$((pass + 1))
  else
    fail=$((fail + 1))
    printf 'FAIL %s\n  expected: %s\n  got:      %s\n' "$name" "$needle" "$haystack" >&2
  fi
}

check_not() {
  local name=$1 haystack=$2 needle=$3
  if [[ $haystack != *"$needle"* ]]; then
    pass=$((pass + 1))
  else
    fail=$((fail + 1))
    printf 'FAIL %s\n  unexpected: %s\n  got:        %s\n' "$name" "$needle" "$haystack" >&2
  fi
}

mkdir -p "$tmp/home" "$tmp/seed"
printf '<settings version="2"/>\n' > "$tmp/seed/guisettings.xml"
printf '<advancedsettings/>\n' > "$tmp/seed/advancedsettings.xml"

# 1. No config file at all: defaults, kodi under gamescope.
out=$(run_session "$tmp/missing.conf")
check "defaults command" "$out" "gamescope -f -- kodi --standalone --windowing=x11"
check "defaults warns" "$(cat "$tmp/stderr")" "no config"

# 2. Kodi profile seeded on first run and left alone afterwards.
check "seed guisettings" "$(ls "$tmp/home/.kodi/userdata")" "guisettings.xml"
check "seed advancedsettings" "$(ls "$tmp/home/.kodi/userdata")" "advancedsettings.xml"
printf 'user-edited\n' > "$tmp/home/.kodi/userdata/guisettings.xml"
run_session "$tmp/missing.conf" >/dev/null
check "seed not overwritten" "$(cat "$tmp/home/.kodi/userdata/guisettings.xml")" "user-edited"

# 3. Full config: resolution, refresh, HDR, extra args, spacing and comments.
cat > "$tmp/full.conf" <<'CONF'
# comment
SHELL = kodi
RESOLUTION=1920x1080
REFRESH = 60
HDR=on
GAMESCOPE_ARGS=--rt --immediate-flips

UNKNOWN=1
CONF
out=$(run_session "$tmp/full.conf")
check "resolution flags" "$out" "-W 1920 -H 1080 -w 1920 -h 1080"
check "refresh flag" "$out" "-r 60"
check "hdr flag" "$out" "--hdr-enabled"
check "extra args" "$out" "--rt --immediate-flips --"
check "unknown key logged" "$(cat "$tmp/stderr")" "unknown key 'UNKNOWN'"

# 4. Invalid values fall back and are logged.
cat > "$tmp/bad.conf" <<'CONF'
SHELL=gnome
RESOLUTION=1920*1080
REFRESH=sixty
HDR=maybe
CONF
out=$(run_session "$tmp/bad.conf")
err=$(cat "$tmp/stderr")
check "bad shell -> kodi" "$out" "-- kodi --standalone"
check_not "bad resolution dropped" "$out" "-W"
check_not "bad refresh dropped" "$out" "-r "
check_not "bad hdr dropped" "$out" "--hdr-enabled"
check "bad shell logged" "$err" "invalid SHELL 'gnome'"
check "bad resolution logged" "$err" "invalid RESOLUTION"
check "bad refresh logged" "$err" "invalid REFRESH"
check "bad hdr logged" "$err" "invalid HDR"

# 5. mpv and custom shells.
printf 'SHELL=mpv\n' > "$tmp/mpv.conf"
out=$(run_session "$tmp/mpv.conf")
check "mpv shell" "$out" "-- mpv --fullscreen"

printf 'SHELL=custom\nSHELL_CMD=/opt/foo --bar=1 --baz\n' > "$tmp/custom.conf"
out=$(run_session "$tmp/custom.conf")
check "custom shell" "$out" "-- bash -c /opt/foo\\ --bar=1\\ --baz"

printf 'SHELL=custom\n' > "$tmp/custom-empty.conf"
out=$(run_session "$tmp/custom-empty.conf")
check "custom without cmd -> kodi" "$out" "-- kodi --standalone"
check "custom without cmd logged" "$(cat "$tmp/stderr")" "SHELL_CMD is empty"

# 6. Values containing '=' survive (only the first '=' splits).
printf 'SHELL=custom\nSHELL_CMD=app --opt=a=b\n' > "$tmp/eq.conf"
out=$(run_session "$tmp/eq.conf")
check "equals in value" "$out" "--opt=a=b"

# 7. CRLF line endings are tolerated.
printf 'SHELL=mpv\r\nHDR=on\r\n' > "$tmp/crlf.conf"
out=$(run_session "$tmp/crlf.conf")
check "crlf shell" "$out" "-- mpv"
check "crlf hdr" "$out" "--hdr-enabled"

printf '%d passed, %d failed\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
