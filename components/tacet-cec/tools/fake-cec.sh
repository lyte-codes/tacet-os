#!/bin/bash
# Integration check without a TV (SPEC.md §10).
#
# libcec has no emulated adapter that this script can drive from outside the
# process, so the "fake" path is the daemon's own --test mode against the
# unit-tested state machine. What this script does:
#
#   1. builds the daemon and runs the unit tests;
#   2. starts `tacet-cec --test` for a few seconds and checks it retries
#      adapter discovery with backoff and shuts down cleanly on SIGTERM;
#   3. checks --list-adapters and --dump-keymap run.
#
# With a real TV attached, use TESTING.md instead: `tacet-cec --test` prints
# every button the TV sends and what it maps to.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

make -s
make -s test

out=$(mktemp)
trap 'rm -f "$out"' EXIT
timeout --preserve-status 3 ./tacet-cec --test --log-level debug >"$out" 2>&1 || true
grep -q 'retrying' "$out" || { echo "fake-cec: expected adapter retry in output"; cat "$out"; exit 1; }
grep -q 'shutting down' "$out" || { echo "fake-cec: expected clean shutdown on SIGTERM"; cat "$out"; exit 1; }

./tacet-cec --list-adapters >/dev/null
./tacet-cec --dump-keymap | grep -q 'KEY_ENTER'
echo "fake-cec: OK (no adapter present; see TESTING.md for the hardware checks)"
