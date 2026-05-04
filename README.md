# FsPluginsOFX

Unofficial OpenFX port bundle for selected bry-ful F's Plugins / NF-Plugins
After Effects effects.

This project is not an official bry-ful release. It is a port intended for OFX
hosts such as Autograph, with the original author's MIT license and credit kept
with the source and binary releases.

The plugins register under the shared OFX group:

```text
F's Plugins OFX
```

## Included Effects

- `FS-SelectiveColorBlur`: NF version
- `FS-ColorSwitch`: NF version
- `FS-RimFill`: NF version
- `FS-LineExtraction`: NF version, first-pass CPU port
- `FS-MainLineRepaint`: Legacy version, because no newer NF project was found
- `FS-SelectColor`: Legacy version, because no newer NF project was found
- `FS-EdgeLine`: Legacy version, because no newer NF project was found

## License Check

The original source repository README says the software is released under the
MIT License, and the repository contains `LICENSE` with the MIT text. Porting
and redistribution are allowed as long as the original copyright notice and
license text are included. This project keeps that notice in
`THIRD_PARTY_NOTICES.md`.

This is a practical engineering check, not legal advice.

## Attribution

Original F's Plugins / NF-Plugins:

- Author: bry-ful / Hiroshi Furuhashi
- Repository: https://github.com/bryful/F-s-PluginsProjects

Credit:

```text
Plugin cooperation: bry-ful
Original F's Plugins / NF-Plugins by bry-ful
```

## Notes

- The NF version is used wherever it exists in the source repository.
- Matching-sensitive effects preserve the original exact or 8-bit tolerance
  style, so color management shifts can affect results.
- Icon assets are kept under `assets/` as optional artwork, but the bundle does
  not currently advertise an icon because host support was inconsistent.
- All effects support RGBA 8-bit, 16-bit, and float OFX images.

`LineExtraction` is ported as a practical first pass around the NF sampling
parameters. Its optional bilateral pre-process from the AE version is not yet
implemented in this OFX build.

## Install

Download the release zip for your platform and place the contained
`FsPluginsOFX.ofx.bundle` into your OFX plugin directory.

Typical OFX locations:

```text
macOS:   /Library/OFX/Plugins/
Windows: C:\Program Files\Common Files\OFX\Plugins\
```

## Build With Make

An OpenFX SDK checkout is required. By default this CMake project looks for it
at `../openfx-sdk`.

```bash
make OPENFX_ROOT=../openfx-sdk
```


## Build With CMake

```bash
cmake -S . -B build -DOPENFX_ROOT=../openfx-sdk
cmake --build build --config Release
ctest --test-dir build -C Release
```

The assembled bundle is written to:

```text
build/FsPluginsOFX.ofx.bundle
```

Install that bundle into your host's OFX plugin directory.
