#!/usr/bin/env python3
"""Generate PNG and iconset assets for the OFX bundle icon."""

from __future__ import annotations

import math
import os
import struct
import subprocess
import zlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets"
ICONSET = ASSETS / "FsPluginsOFX.iconset"


def mix(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def rgba(r: float, g: float, b: float, a: float = 1.0) -> tuple[int, int, int, int]:
    return (
        max(0, min(255, round(r))),
        max(0, min(255, round(g))),
        max(0, min(255, round(b))),
        max(0, min(255, round(a * 255))),
    )


def blend(dst: tuple[int, int, int, int], src: tuple[int, int, int, int]) -> tuple[int, int, int, int]:
    sr, sg, sb, sa = src
    dr, dg, db, da = dst
    a = sa / 255.0
    out_a = a + (da / 255.0) * (1.0 - a)
    if out_a <= 0.0:
        return (0, 0, 0, 0)
    return (
        round((sr * a + dr * (da / 255.0) * (1.0 - a)) / out_a),
        round((sg * a + dg * (da / 255.0) * (1.0 - a)) / out_a),
        round((sb * a + db * (da / 255.0) * (1.0 - a)) / out_a),
        round(out_a * 255),
    )


class Image:
    def __init__(self, width: int, height: int) -> None:
        self.width = width
        self.height = height
        self.pixels = [(0, 0, 0, 0)] * (width * height)

    def set(self, x: int, y: int, color: tuple[int, int, int, int]) -> None:
        if 0 <= x < self.width and 0 <= y < self.height:
            self.pixels[y * self.width + x] = blend(self.pixels[y * self.width + x], color)

    def rect(self, x0: int, y0: int, x1: int, y1: int, color: tuple[int, int, int, int]) -> None:
        for y in range(max(0, y0), min(self.height, y1)):
            for x in range(max(0, x0), min(self.width, x1)):
                self.set(x, y, color)

    def rounded_rect(self, x0: int, y0: int, x1: int, y1: int, radius: int) -> None:
        for y in range(max(0, y0), min(self.height, y1)):
            for x in range(max(0, x0), min(self.width, x1)):
                cx = min(max(x, x0 + radius), x1 - radius - 1)
                cy = min(max(y, y0 + radius), y1 - radius - 1)
                if (x - cx) * (x - cx) + (y - cy) * (y - cy) <= radius * radius:
                    t = (x - x0 + y - y0) / max(1, (x1 - x0) + (y1 - y0))
                    self.set(x, y, rgba(mix(36, 17, t), mix(40, 19, t), mix(47, 23, t), 1.0))

    def save_png(self, path: Path) -> None:
        rows = []
        for y in range(self.height):
            row = bytearray([0])
            for r, g, b, a in self.pixels[y * self.width : (y + 1) * self.width]:
                row.extend([r, g, b, a])
            rows.append(bytes(row))
        raw = b"".join(rows)

        def chunk(name: bytes, data: bytes) -> bytes:
            return struct.pack(">I", len(data)) + name + data + struct.pack(">I", zlib.crc32(name + data) & 0xFFFFFFFF)

        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(
            b"\x89PNG\r\n\x1a\n"
            + chunk(b"IHDR", struct.pack(">IIBBBBB", self.width, self.height, 8, 6, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(raw, 9))
            + chunk(b"IEND", b"")
        )


def draw_icon(size: int) -> Image:
    img = Image(size, size)
    s = size / 512.0
    img.rounded_rect(round(48 * s), round(48 * s), round(464 * s), round(464 * s), round(88 * s))

    cyan = rgba(86, 215, 255, 1.0)
    pink = rgba(255, 122, 182, 1.0)
    yellow = rgba(255, 209, 102, 1.0)
    white = rgba(247, 251, 255, 1.0)

    # F
    img.rect(round(128 * s), round(150 * s), round(188 * s), round(404 * s), cyan)
    img.rect(round(128 * s), round(150 * s), round(278 * s), round(204 * s), cyan)
    img.rect(round(128 * s), round(252 * s), round(270 * s), round(304 * s), cyan)

    # Apostrophe
    img.rect(round(286 * s), round(116 * s), round(342 * s), round(170 * s), white)
    img.rect(round(304 * s), round(160 * s), round(326 * s), round(218 * s), white)

    # S built from bold horizontal and vertical strokes.
    img.rect(round(250 * s), round(224 * s), round(390 * s), round(278 * s), pink)
    img.rect(round(250 * s), round(224 * s), round(306 * s), round(326 * s), pink)
    img.rect(round(250 * s), round(306 * s), round(382 * s), round(358 * s), yellow)
    img.rect(round(326 * s), round(306 * s), round(382 * s), round(410 * s), yellow)
    img.rect(round(238 * s), round(356 * s), round(382 * s), round(410 * s), yellow)

    img.rect(round(112 * s), round(423 * s), round(400 * s), round(437 * s), white)
    return img


def main() -> None:
    ASSETS.mkdir(parents=True, exist_ok=True)
    ICONSET.mkdir(parents=True, exist_ok=True)

    base = draw_icon(1024)
    base.save_png(ASSETS / "fs-plugins-ofx-icon.png")

    sizes = {
        "icon_16x16.png": 16,
        "icon_16x16@2x.png": 32,
        "icon_32x32.png": 32,
        "icon_32x32@2x.png": 64,
        "icon_128x128.png": 128,
        "icon_128x128@2x.png": 256,
        "icon_256x256.png": 256,
        "icon_256x256@2x.png": 512,
        "icon_512x512.png": 512,
        "icon_512x512@2x.png": 1024,
    }
    for name, size in sizes.items():
        draw_icon(size).save_png(ICONSET / name)

    icns = ASSETS / "FsPluginsOFX.icns"
    iconutil = Path("/usr/bin/iconutil")
    if iconutil.exists():
        result = subprocess.run(
            [str(iconutil), "-c", "icns", str(ICONSET), "-o", str(icns)],
            check=False,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        if result.returncode == 0:
            return

    # Some sandboxed macOS environments reject generated iconsets even when the
    # PNG files are valid. tiff2icns is less strict and still produces a usable
    # Finder/bundle icon.
    tiff2icns = Path("/usr/bin/tiff2icns")
    sips = Path("/usr/bin/sips")
    if tiff2icns.exists() and sips.exists():
        tmp_tiff = ASSETS / "FsPluginsOFX.tiff"
        subprocess.run(
            [str(sips), "-s", "format", "tiff", str(ASSETS / "fs-plugins-ofx-icon.png"), "--out", str(tmp_tiff)],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        subprocess.run([str(tiff2icns), str(tmp_tiff), str(icns)], check=True)
        tmp_tiff.unlink(missing_ok=True)


if __name__ == "__main__":
    main()
