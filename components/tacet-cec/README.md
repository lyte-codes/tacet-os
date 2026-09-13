# tacet-cec

CEC → uinput bridge. Milestone M1. **Not started.**

The spec (§4.3) says this component is fully specified in
`tacet-cec-bridge-spec.md`, but that file is not in the repository. Nothing
here is guessed from the one-line description: add the spec as
`components/tacet-cec/SPEC.md` and the implementation follows it.

What already exists for it:

- user `tacet-cec` (groups `tacet`, `input`, `video`, `dialout`) from
  `overlays/usr/lib/sysusers.d/tacet.conf`;
- `libcec` and `v4l-utils` (`cec-ctl`) in the image;
- `tacet-boot-report` probes `tacet-cec --list-adapters` when the binary is
  present, which is the CI assertion from spec §8;
- the Containerfile's unit-enable step has a place for `tacet-cec.service`.
