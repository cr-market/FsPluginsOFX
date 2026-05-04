# Release Notes

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

Tested by the maintainer:

- macOS: Autograph smoke test passed
- Windows x64: Autograph smoke test passed

Known limitations:

- `FS-LineExtraction` is a first-pass CPU port of the NF sampling/level/blend
  behavior. The original optional bilateral pre-process is not implemented yet.
- The `FS-` prefix is used so Autograph keeps these modifiers grouped in a
  name-sorted modifier list.
- Icon advertisement is disabled because host support was inconsistent.

License and attribution:

- This project is distributed under the MIT License.
- Original F's Plugins / NF-Plugins code and behavior are by bry-ful / Hiroshi
  Furuhashi and are also MIT-licensed. See `THIRD_PARTY_NOTICES.md`.
