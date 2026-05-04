# Release Notes

## Unreleased

- Added the NF-style optional bilateral pre-processing controls to
  `FS-LineExtraction`.
- Matched `FS-LineExtraction` parameter ranges and defaults more closely to the
  After Effects version, including 0-100 UI scales for sampling, Sigma Range,
  and post levels.
- Changed `FS-LineExtraction` `Outer Sampling` and `Inner Sampling` display
  ranges to 0-100 so users can match After Effects settings directly.
- Added `FS-Thin` as a CPU OFX port based on the NF version's target color,
  thinning controls, and pattern thinning passes.
- Matched several existing plugin defaults/ranges to their After Effects
  versions: `FS-ColorSwitch`, `FS-RimFill`, `FS-MainLineRepaint`, and
  `FS-EdgeLine`.
- Removed the previous `FS-LineExtraction` bilateral limitation note.

## v0.1.0

Initial public test release of `FsPluginsOFX`, an unofficial OpenFX port bundle
based on selected bry-ful F's Plugins / NF-Plugins effects.

Included effects:

- `FS-ColorSwitch`
- `FS-EdgeLine`
- `FS-LineExtraction`
- `FS-MainLineRepaint`
- `FS-RimFill`
- `FS-SelectColor`
- `FS-SelectiveColorBlur`
- `FS-Thin`

Tested by the maintainer:

- macOS: Autograph smoke test passed
- Windows x64: Autograph smoke test passed

Known limitations:

- The `FS-` prefix is used so Autograph keeps these modifiers grouped in a
  name-sorted modifier list.
- Icon advertisement is disabled because host support was inconsistent.

License and attribution:

- This project is distributed under the MIT License.
- Original F's Plugins / NF-Plugins code and behavior are by bry-ful / Hiroshi
  Furuhashi and are also MIT-licensed. See `THIRD_PARTY_NOTICES.md`.
