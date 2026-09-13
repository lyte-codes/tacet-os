# Hardware

| Tier | Device | Status |
|---|---|---|
| 1 | Intel N100 mini PC (any; reference: a common 8GB/256GB unit) | Primary target. UEFI, VA-API via `intel-media-driver`, mainline kernel. |
| 2 | Raspberry Pi 5 (4GB+) | Secondary. Blocked on bootc's ARM boot chain; tracked as an issue. |
| 3 | RISC-V (riscv64) | Planned. Blocked on a board with mainline hardware video decode and a Fedora riscv64 bootc base. |
| — | Anything else | Community, unsupported. |

## Requirements for any target

- UEFI boot (or a bootc-supported boot chain).
- Hardware H.264/HEVC/AV1 decode with an open driver.
- HDMI-CEC reachable from Linux: the kernel `cec` framework on a supported
  GPU/board, or a Pulse-Eight USB-CEC adapter. Most N100 boxes have no
  CEC on their HDMI port; the Pulse-Eight adapter is the reliable answer.

## N100 notes

- The `tv` user runs gamescope directly on DRM; no X server, no display
  manager.
- `intel-media-driver` (iHD) provides VA-API; `vainfo` in the image shows
  the decode profiles.
- 8 GB RAM is plenty; 4 GB works. Storage: the image needs about 10 GB; the
  rest of the disk is left for `/var`.

## CEC

`tacet-cec` (M1) drives the TV remote. It needs one of:

- a **Pulse-Eight USB-CEC adapter** between the box and the TV. Shows up as
  `/dev/ttyACM0` (symlinked `/dev/tacet-cec-adapter`). This is the N100 path;
  Intel HDMI ports do not expose CEC.
- a board with **native CEC** through the kernel `cec` framework
  (`/dev/cec0`): Raspberry Pi 5, and some boards via `cec-gpio`.

`tacet-cec --list-adapters` shows what libcec can see; `tacet-cec --test`
shows what the TV sends. See `components/tacet-cec/TESTING.md`.

## Checking a box after flashing

From a keyboard on the box (SSH is off by default):

```
journalctl -b -u tacet-session      # gamescope + Kodi startup
loginctl list-sessions               # `tv` on seat0, tty1
vainfo                               # hardware decode profiles
tacet-cec --list-adapters            # CEC adapters libcec can see
systemctl status tacet-cec           # the bridge; --test mode in components/tacet-cec/TESTING.md
```
