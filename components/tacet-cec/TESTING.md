# Testing tacet-cec

## Without hardware

```
make test              # unit tests: keymap, config, repeat state machine
tools/fake-cec.sh      # build + tests + backoff/shutdown behaviour of the daemon
```

CI runs both, and the image boot test runs `tacet-cec --list-adapters`
inside the built image (no adapter in QEMU is a pass).

## With a TV (acceptance, SPEC.md §10)

Box with a CEC path: a Pulse-Eight USB-CEC adapter on an N100, or native
`/dev/cec0` on a Raspberry Pi 5. TV with CEC enabled (Samsung "Anynet+",
LG "SimpLink", Sony "Bravia Sync", Philips "EasyLink", and so on).

First, see what the TV sends. From a shell on the box (SSH is off by default
in the image; use a keyboard on tty2 or enable SSH in Settings when M4 lands):

```
sudo systemctl stop tacet-cec
sudo -u tacet-cec tacet-cec --test
```

Press every button on the remote. Each line shows the CEC code, its name,
and the key it maps to, or `unmapped`. Paste that output into any bug report.
Start the service again with `sudo systemctl start tacet-cec`.

Then the acceptance list. Tick every one before calling M1 done:

1. **Navigation.** Fresh boot, no keyboard attached. The TV remote moves
   through Kodi's menus; Select opens, Exit goes back, Play/Pause/Stop work
   in a video. `sudo libinput list-devices` shows `Tacet CEC Keyboard` and
   `Tacet CEC Gamepad`.
2. **Repeat.** Hold Down in a long list: it starts scrolling after roughly
   400 ms at about 10 rows per second and stops the moment you let go. No
   double-steps on a single press.
3. **HDMI replug.** Unplug the HDMI cable (or the USB adapter), wait ten
   seconds, plug it back. Within five seconds the remote works again;
   nothing is stuck held. `journalctl -u tacet-cec` shows the disconnect,
   the retry, and the reconnect.
4. **TV power cycle.** Turn the TV off and on with its remote. The TV
   returns to the box's input (Active Source re-claimed) and the remote
   works without restarting the service.
5. **Idle.** After 24 hours idle, `journalctl -u tacet-cec -p warning`
   is empty.

Plus the two checks the spec leaves open (§12):

6. **Info and colour keys.** Do Display Info and Red/Green/Yellow/Blue do
   anything in Kodi? Under XWayland they will not (`README.md`, known
   limitation). Record which, and whether the `cec.conf.default` overrides
   fix it.
7. **Gamepad recognition.** In an app that uses controllers (Moonlight, or
   `sudo evtest` on the gamepad node), does the d-pad hat register? If
   gamescope ignores the device, try Xbox-style vendor/product IDs in
   `src/uinput.c` and record the result.

## Things worth knowing when it does not work

- `tacet-cec --list-adapters` prints nothing useful → libcec cannot see the
  adapter. Pulse-Eight: `ls -l /dev/ttyACM* /dev/tacet-cec-adapter` and check
  the group is `input`. Pi 5: `ls -l /dev/cec*`.
- Adapter found, no keys arrive → the TV is not routing keys to us. Check the
  TV's CEC setting is on, then `--log-level debug` to see whether the TV
  answers our Active Source. `physical_address = 1.0.0.0` pins the port if
  autodetection picks the wrong one.
- Keys arrive in `--test` but Kodi ignores them → `sudo libinput
  list-devices` should list the keyboard with `Capabilities: keyboard`; if
  not, the udev rule did not apply (`udevadm info /dev/input/eventN`).
- Cannot create the virtual keyboard, exit 78 → `/dev/uinput` permissions.
  `ls -l /dev/uinput` should show group `input` mode 0660.
