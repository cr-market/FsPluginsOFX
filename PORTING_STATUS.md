# Porting Status

All plugins are registered in the shared OFX group `F's Plugins OFX`.

| OFX Effect | Preferred Source Used | Status |
| --- | --- | --- |
| FS-SelectiveColorBlur | `SelectiveColorBlur` NF project | Built |
| FS-ColorSwitch | `ColorSwitch` NF project | Built |
| FS-RimFill | `RimFill` NF project | Built |
| FS-LineExtraction | `LineExtraction` NF project | Built, including bilateral pre-processing |
| FS-Thin | `Thin` NF project | Built, including NF pattern thinning passes |
| FS-MainLineRepaint | `Legacy/Project/MainLineRepaint` | Built from Legacy logic; no NF project found |
| FS-SelectColor | `Legacy/Project/SelectColor` | Built from Legacy logic; no NF project found |
| FS-EdgeLine | `Legacy/Project/EdgeLine` | Built from Legacy logic; no NF project found |

## Notes

- New NF projects are preferred when present in the source repository.
- `LineExtraction` ports the NF-style bilateral pre-process plus the core
  sampling/level/blend behavior. The OFX implementation is CPU-only.
- Icon artwork exists under `assets/`, but icon advertisement is disabled for
  now because host support was inconsistent.
- Host-side visual parity still needs checking in Resolve/Nuke/Fusion or the
  target OFX host.
