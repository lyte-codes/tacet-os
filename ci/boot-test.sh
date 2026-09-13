#!/bin/bash
# Boot a raw Tacet disk in QEMU and check the session comes up.
#
#   ci/boot-test.sh <disk.raw> [timeout-seconds]
#
# Passes when the serial log contains a TACET-BOOT-REPORT line with
# session=active and cec!=fail (spec §8). The report is written by
# tacet-boot-report.service, which only runs under virtualization. No kernel
# arguments are injected, so the disk under test is byte-identical to what a
# user would flash.
set -euo pipefail

disk=${1:?usage: boot-test.sh <disk.raw> [timeout]}
timeout=${2:-420}
log=${BOOT_TEST_LOG:-serial.log}
QEMU=${QEMU:-qemu-system-x86_64}

find_ovmf() {
  local c
  for c in /usr/share/OVMF/OVMF_CODE_4M.fd /usr/share/OVMF/OVMF_CODE.fd \
           /usr/share/edk2/ovmf/OVMF_CODE.fd /usr/share/edk2-ovmf/x64/OVMF_CODE.4m.fd \
           /usr/share/qemu/ovmf-x86_64-code.bin; do
    [ -r "$c" ] && { echo "$c"; return; }
  done
  echo "boot-test: no OVMF firmware found" >&2
  exit 2
}

ovmf=$(find_ovmf)
accel=()
if [ -w /dev/kvm ]; then
  accel=(-enable-kvm -cpu host)
else
  echo "boot-test: /dev/kvm not writable, running without KVM (slow)" >&2
  accel=(-cpu max)
fi

: > "$log"
"$QEMU" \
  "${accel[@]}" -m 2048 -smp 2 \
  -machine q35 \
  -drive "if=pflash,format=raw,readonly=on,file=$ovmf" \
  -drive "file=$disk,format=raw,if=virtio" \
  -device virtio-vga \
  -device virtio-net-pci,netdev=n0 -netdev user,id=n0 \
  -display none \
  -serial "file:$log" \
  -monitor none \
  -no-reboot &
qemu_pid=$!
trap 'kill "$qemu_pid" 2>/dev/null || true' EXIT

report=""
for ((i = 0; i < timeout; i++)); do
  if ! kill -0 "$qemu_pid" 2>/dev/null; then
    echo "boot-test: QEMU exited early" >&2
    break
  fi
  report=$(grep -a -m1 'TACET-BOOT-REPORT' "$log" || true)
  [ -n "$report" ] && break
  sleep 1
done

echo "boot-test: ${report:-no report within ${timeout}s}"
if [ -z "$report" ]; then
  echo "--- last 40 lines of $log ---" >&2
  tail -n 40 "$log" >&2 || true
  exit 1
fi
[[ $report == *"session=active"* ]] || { echo "boot-test: session not active" >&2; exit 1; }
[[ $report != *"cec=fail"* ]] || { echo "boot-test: tacet-cec --list-adapters crashed" >&2; exit 1; }
echo "boot-test: PASS"
