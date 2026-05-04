# FsPluginsOFX v0.1.0

Initial public test release of `FsPluginsOFX`, an unofficial OpenFX port bundle
based on selected bry-ful F's Plugins / NF-Plugins effects.

## Included effects

- `FS-ColorSwitch`
- `FS-EdgeLine`
- `FS-LineExtraction`
- `FS-MainLineRepaint`
- `FS-RimFill`
- `FS-SelectColor`
- `FS-SelectiveColorBlur`

## Downloads

- `FsPluginsOFX-macOS-arm64.zip`
- `FsPluginsOFX-Windows-x64.zip`

Each binary zip includes the OFX bundle plus `README.md`, `LICENSE`,
`THIRD_PARTY_NOTICES.md`, and `RELEASE_NOTES.md`.

## Attribution and license

Original F's Plugins / NF-Plugins are by bry-ful / Hiroshi Furuhashi:

https://github.com/bryful/F-s-PluginsProjects

This port is distributed under the MIT License. Original MIT copyright and
license notice are included in `THIRD_PARTY_NOTICES.md`.

Suggested credit:

```text
Plugin cooperation: bry-ful
Original F's Plugins / NF-Plugins by bry-ful
```

## Notes

- The `FS-` prefix is used so name-sorted OFX hosts such as Autograph keep these
  modifiers together.
- `FS-LineExtraction` is a first-pass CPU port of the NF sampling/level/blend
  behavior. The original optional bilateral pre-process is not implemented yet.
- Icon advertisement is disabled because host support was inconsistent.
