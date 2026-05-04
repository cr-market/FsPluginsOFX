// SelectiveColorBlur OFX port.
// Algorithm adapted from bryful/F-s-PluginsProjects SelectiveColorBlur (MIT).

#include "SelectiveColorBlurCore.h"

#include <algorithm>
#include <cmath>

namespace selective_color_blur {
namespace {

constexpr float kMaskOn = 1.0f;

bool isTarget(const PixelF& pixel, const std::vector<Color>& targets) {
  const std::uint8_t r = quantize8(pixel.r);
  const std::uint8_t g = quantize8(pixel.g);
  const std::uint8_t b = quantize8(pixel.b);

  for (const Color& target : targets) {
    if (r == quantize8(target.r) && g == quantize8(target.g) && b == quantize8(target.b)) {
      return true;
    }
  }
  return false;
}

PixelF transparentWhite() {
  return {1.0f, 1.0f, 1.0f, 0.0f};
}

bool isTransparentLike(const PixelF& pixel, bool whiteIsTransparent) {
  if (pixel.a <= 0.0f) {
    return true;
  }
  return whiteIsTransparent && quantize8(pixel.r) == 255 && quantize8(pixel.g) == 255 && quantize8(pixel.b) == 255;
}

bool isMainLine(const PixelF& pixel, const MainLineRepaintParams& params) {
  return pixel.a > 0.0f && matchesColor(pixel, params.mainColor, params.tolerance8);
}

PixelF chooseReplacement(const std::vector<PixelF>& line, int center, int scanLength, const MainLineRepaintParams& params) {
  int left = center;
  int right = center;
  for (int k = 1; k < scanLength; ++k) {
    const int p = center - k;
    if (p < 0) {
      break;
    }
    if (!isMainLine(line[static_cast<std::size_t>(p)], params)) {
      left = p;
      break;
    }
  }
  for (int k = 1; k < scanLength; ++k) {
    const int p = center + k;
    if (p >= static_cast<int>(line.size())) {
      break;
    }
    if (!isMainLine(line[static_cast<std::size_t>(p)], params)) {
      right = p;
      break;
    }
  }

  const PixelF& l = line[static_cast<std::size_t>(left)];
  const PixelF& r = line[static_cast<std::size_t>(right)];
  if (isTransparentLike(l, false) && isTransparentLike(r, false)) {
    return {0.0f, 0.0f, 0.0f, 0.0f};
  }
  if (isTransparentLike(l, false)) {
    return r;
  }
  if (isTransparentLike(r, false)) {
    return l;
  }
  if (isMainLine(l, params) && !isMainLine(r, params)) {
    return r;
  }
  if (!isMainLine(l, params) && isMainLine(r, params)) {
    return l;
  }
  return luminance(l) <= luminance(r) ? l : r;
}

std::vector<double> makeWeights(int radius) {
  std::vector<double> weights(static_cast<std::size_t>(radius) + 1U, 1.0);
  if (radius <= 0) {
    return weights;
  }

  const double zone = static_cast<double>(radius) / 3.0;
  const double denom = 2.0 * zone * zone;
  for (int k = 0; k <= radius; ++k) {
    weights[static_cast<std::size_t>(k)] = std::exp(-(static_cast<double>(k) * k) / denom);
  }
  return weights;
}

void horizontalBlur(std::vector<PixelF>& data, int width, int height, int radius, const std::vector<double>& weights) {
  if (radius <= 0) {
    return;
  }

  std::vector<PixelF> scanline(static_cast<std::size_t>(width));
  for (int y = 0; y < height; ++y) {
    PixelF* row = data.data() + static_cast<std::size_t>(y) * width;
    std::copy(row, row + width, scanline.begin());

    for (int x = 0; x < width; ++x) {
      if (scanline[static_cast<std::size_t>(x)].a != kMaskOn) {
        continue;
      }

      double sr = 0.0;
      double sg = 0.0;
      double sb = 0.0;
      double weightSum = 0.0;

      const PixelF& center = scanline[static_cast<std::size_t>(x)];
      double weight = weights[0];
      sr += static_cast<double>(center.r) * weight;
      sg += static_cast<double>(center.g) * weight;
      sb += static_cast<double>(center.b) * weight;
      weightSum += weight;

      for (int offset = 1; offset <= radius; ++offset) {
        const int sampleX = x - offset;
        if (sampleX < 0) {
          break;
        }
        const PixelF& sample = scanline[static_cast<std::size_t>(sampleX)];
        if (sample.a != kMaskOn) {
          break;
        }

        weight = weights[static_cast<std::size_t>(offset)];
        sr += static_cast<double>(sample.r) * weight;
        sg += static_cast<double>(sample.g) * weight;
        sb += static_cast<double>(sample.b) * weight;
        weightSum += weight;
      }

      for (int offset = 1; offset <= radius; ++offset) {
        const int sampleX = x + offset;
        if (sampleX >= width) {
          break;
        }
        const PixelF& sample = scanline[static_cast<std::size_t>(sampleX)];
        if (sample.a != kMaskOn) {
          break;
        }

        weight = weights[static_cast<std::size_t>(offset)];
        sr += static_cast<double>(sample.r) * weight;
        sg += static_cast<double>(sample.g) * weight;
        sb += static_cast<double>(sample.b) * weight;
        weightSum += weight;
      }

      if (weightSum > 0.0) {
        row[x].r = static_cast<float>(sr / weightSum);
        row[x].g = static_cast<float>(sg / weightSum);
        row[x].b = static_cast<float>(sb / weightSum);
      }
    }
  }
}

void verticalBlur(std::vector<PixelF>& data, int width, int height, int radius, const std::vector<double>& weights) {
  if (radius <= 0) {
    return;
  }

  std::vector<PixelF> scanline(static_cast<std::size_t>(height));
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      scanline[static_cast<std::size_t>(y)] = data[static_cast<std::size_t>(y) * width + x];
    }

    for (int y = 0; y < height; ++y) {
      if (scanline[static_cast<std::size_t>(y)].a != kMaskOn) {
        continue;
      }

      double sr = 0.0;
      double sg = 0.0;
      double sb = 0.0;
      double weightSum = 0.0;

      const PixelF& center = scanline[static_cast<std::size_t>(y)];
      double weight = weights[0];
      sr += static_cast<double>(center.r) * weight;
      sg += static_cast<double>(center.g) * weight;
      sb += static_cast<double>(center.b) * weight;
      weightSum += weight;

      for (int offset = 1; offset <= radius; ++offset) {
        const int sampleY = y - offset;
        if (sampleY < 0) {
          break;
        }
        const PixelF& sample = scanline[static_cast<std::size_t>(sampleY)];
        if (sample.a != kMaskOn) {
          break;
        }

        weight = weights[static_cast<std::size_t>(offset)];
        sr += static_cast<double>(sample.r) * weight;
        sg += static_cast<double>(sample.g) * weight;
        sb += static_cast<double>(sample.b) * weight;
        weightSum += weight;
      }

      for (int offset = 1; offset <= radius; ++offset) {
        const int sampleY = y + offset;
        if (sampleY >= height) {
          break;
        }
        const PixelF& sample = scanline[static_cast<std::size_t>(sampleY)];
        if (sample.a != kMaskOn) {
          break;
        }

        weight = weights[static_cast<std::size_t>(offset)];
        sr += static_cast<double>(sample.r) * weight;
        sg += static_cast<double>(sample.g) * weight;
        sb += static_cast<double>(sample.b) * weight;
        weightSum += weight;
      }

      if (weightSum > 0.0) {
        PixelF& out = data[static_cast<std::size_t>(y) * width + x];
        out.r = static_cast<float>(sr / weightSum);
        out.g = static_cast<float>(sg / weightSum);
        out.b = static_cast<float>(sb / weightSum);
      }
    }
  }
}

}  // namespace

std::uint8_t quantize8(float value) {
  const float clamped = std::clamp(value, 0.0f, 1.0f);
  return static_cast<std::uint8_t>(std::lround(clamped * 255.0f));
}

bool matchesColor(const PixelF& pixel, const Color& color, int tolerance8) {
  const int tol = std::clamp(tolerance8, 0, 255);
  return std::abs(static_cast<int>(quantize8(pixel.r)) - static_cast<int>(quantize8(color.r))) <= tol &&
         std::abs(static_cast<int>(quantize8(pixel.g)) - static_cast<int>(quantize8(color.g))) <= tol &&
         std::abs(static_cast<int>(quantize8(pixel.b)) - static_cast<int>(quantize8(color.b))) <= tol;
}

float luminance(const PixelF& pixel) {
  return 0.29891f * pixel.r + 0.58661f * pixel.g + 0.11448f * pixel.b;
}

void apply(const PixelF* src, PixelF* dst, int width, int height, const Parameters& params) {
  if (!src || !dst || width <= 0 || height <= 0) {
    return;
  }

  std::copy(src, src + static_cast<std::size_t>(width) * height, dst);
  if (params.radius <= 0) {
    return;
  }

  std::vector<Color> targets;
  for (const TargetColor& target : params.targets) {
    if (target.enabled) {
      targets.push_back(target.color);
    }
  }
  if (targets.empty()) {
    return;
  }

  std::vector<PixelF> selected(static_cast<std::size_t>(width) * height);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const std::size_t index = static_cast<std::size_t>(y) * width + x;
      if (isTarget(src[index], targets)) {
        selected[index] = src[index];
        selected[index].a = kMaskOn;
      }
    }
  }

  const int radius = std::min(params.radius, 1023);
  const std::vector<double> weights = makeWeights(radius);

  int passRadius = radius;
  horizontalBlur(selected, width, height, passRadius, weights);
  verticalBlur(selected, width, height, passRadius, weights);
  passRadius /= 4;
  horizontalBlur(selected, width, height, passRadius, weights);
  passRadius /= 4;
  verticalBlur(selected, width, height, passRadius, weights);
  passRadius /= 4;
  horizontalBlur(selected, width, height, passRadius, weights);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const std::size_t index = static_cast<std::size_t>(y) * width + x;
      if (selected[index].a == kMaskOn) {
        dst[index] = selected[index];
        dst[index].a = src[index].a;
      }
    }
  }
}

void applySelectColor(const PixelF* src, PixelF* dst, int width, int height, const SelectColorParams& params) {
  if (!src || !dst || width <= 0 || height <= 0) {
    return;
  }
  std::copy(src, src + static_cast<std::size_t>(width) * height, dst);

  std::vector<Color> targets;
  for (const TargetColor& target : params.targets) {
    if (target.enabled) {
      targets.push_back(target.color);
    }
  }
  if (targets.empty()) {
    return;
  }

  for (int i = 0; i < width * height; ++i) {
    bool matched = false;
    for (const Color& target : targets) {
      if (matchesColor(src[i], target, params.tolerance8)) {
        matched = true;
        break;
      }
    }
    if (matched == params.reverse) {
      dst[i] = transparentWhite();
    }
  }
}

void applyColorSwitch(const PixelF* src, PixelF* dst, int width, int height, const ColorSwitchParams& params) {
  if (!src || !dst || width <= 0 || height <= 0) {
    return;
  }
  std::copy(src, src + static_cast<std::size_t>(width) * height, dst);
  if (!params.enableAll) {
    return;
  }

  const int count = std::clamp(params.activeCount, 0, static_cast<int>(params.entries.size()));
  for (int p = 0; p < width * height; ++p) {
    int match = -1;
    for (int i = 0; i < count; ++i) {
      const ColorSwitchEntry& entry = params.entries[static_cast<std::size_t>(i)];
      if (entry.enabled && matchesColor(src[p], entry.oldColor, 0)) {
        match = i;
        break;
      }
    }

    if (params.mode == ColorSwitchMode::Replace) {
      if (match >= 0) {
        const Color& c = params.entries[static_cast<std::size_t>(match)].newColor;
        dst[p] = {c.r, c.g, c.b, src[p].a};
      }
    } else if (params.mode == ColorSwitchMode::Key) {
      if (match >= 0) {
        dst[p].a = 0.0f;
      }
    } else if (params.mode == ColorSwitchMode::Extract) {
      if (match < 0) {
        dst[p].a = 0.0f;
      }
    }
  }
}

void applyEdgeLine(const PixelF* src, PixelF* dst, int width, int height, const EdgeLineParams& params) {
  if (!src || !dst || width <= 0 || height <= 0) {
    return;
  }
  std::copy(src, src + static_cast<std::size_t>(width) * height, dst);
  if (params.length <= 0) {
    return;
  }

  const int radius = std::clamp(params.length, 0, 200);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const std::size_t index = static_cast<std::size_t>(y) * width + x;
      if (!matchesColor(src[index], params.targetColor, params.tolerance8)) {
        continue;
      }

      bool found = false;
      for (int yy = std::max(0, y - radius); yy <= std::min(height - 1, y + radius) && !found; ++yy) {
        for (int xx = std::max(0, x - radius); xx <= std::min(width - 1, x + radius); ++xx) {
          if (matchesColor(src[static_cast<std::size_t>(yy) * width + xx], params.sampleColor, params.tolerance8)) {
            found = true;
            break;
          }
        }
      }

      if (found) {
        dst[index] = {params.drawColor.r, params.drawColor.g, params.drawColor.b, params.drawColor.a};
      }
    }
  }
}

void applyRimFill(const PixelF* src, PixelF* dst, int width, int height, const RimFillParams& params) {
  if (!src || !dst || width <= 0 || height <= 0) {
    return;
  }
  std::copy(src, src + static_cast<std::size_t>(width) * height, dst);
  const int fillWidth = std::clamp(params.width, 0, 200);
  if (fillWidth <= 0) {
    return;
  }

  auto fillPixel = [&](const PixelF& adjacent) {
    if (params.mode == RimFillMode::AdjacentColor) {
      return PixelF{adjacent.r, adjacent.g, adjacent.b, 1.0f};
    }
    return PixelF{params.customColor.r, params.customColor.g, params.customColor.b, params.customColor.a};
  };

  for (int y = 0; y < height; ++y) {
    std::vector<PixelF> line(static_cast<std::size_t>(width));
    for (int x = 0; x < width; ++x) {
      line[static_cast<std::size_t>(x)] = dst[static_cast<std::size_t>(y) * width + x];
    }
    for (int x = 0; x < width; ++x) {
      if (!isTransparentLike(line[static_cast<std::size_t>(x)], params.whiteIsTransparent)) {
        continue;
      }
      PixelF col;
      bool has = false;
      if (x + 1 < width && !isTransparentLike(line[static_cast<std::size_t>(x + 1)], params.whiteIsTransparent)) {
        col = fillPixel(line[static_cast<std::size_t>(x + 1)]);
        has = true;
        for (int k = 0; k <= fillWidth && x - k >= 0; ++k) {
          if (!isTransparentLike(line[static_cast<std::size_t>(x - k)], params.whiteIsTransparent)) break;
          dst[static_cast<std::size_t>(y) * width + (x - k)] = col;
        }
      }
      if (!has && x > 0 && !isTransparentLike(line[static_cast<std::size_t>(x - 1)], params.whiteIsTransparent)) {
        col = fillPixel(line[static_cast<std::size_t>(x - 1)]);
        for (int k = 0; k <= fillWidth && x + k < width; ++k) {
          if (!isTransparentLike(line[static_cast<std::size_t>(x + k)], params.whiteIsTransparent)) break;
          dst[static_cast<std::size_t>(y) * width + (x + k)] = col;
        }
      }
    }
  }

  for (int x = 0; x < width; ++x) {
    std::vector<PixelF> line(static_cast<std::size_t>(height));
    for (int y = 0; y < height; ++y) {
      line[static_cast<std::size_t>(y)] = dst[static_cast<std::size_t>(y) * width + x];
    }
    for (int y = 0; y < height; ++y) {
      if (!isTransparentLike(line[static_cast<std::size_t>(y)], params.whiteIsTransparent)) {
        continue;
      }
      PixelF col;
      bool has = false;
      if (y + 1 < height && !isTransparentLike(line[static_cast<std::size_t>(y + 1)], params.whiteIsTransparent)) {
        col = fillPixel(line[static_cast<std::size_t>(y + 1)]);
        has = true;
        for (int k = 0; k <= fillWidth && y - k >= 0; ++k) {
          if (!isTransparentLike(line[static_cast<std::size_t>(y - k)], params.whiteIsTransparent)) break;
          dst[static_cast<std::size_t>(y - k) * width + x] = col;
        }
      }
      if (!has && y > 0 && !isTransparentLike(line[static_cast<std::size_t>(y - 1)], params.whiteIsTransparent)) {
        col = fillPixel(line[static_cast<std::size_t>(y - 1)]);
        for (int k = 0; k <= fillWidth && y + k < height; ++k) {
          if (!isTransparentLike(line[static_cast<std::size_t>(y + k)], params.whiteIsTransparent)) break;
          dst[static_cast<std::size_t>(y + k) * width + x] = col;
        }
      }
    }
  }
}

void applyMainLineRepaint(const PixelF* src, PixelF* dst, int width, int height, const MainLineRepaintParams& params) {
  if (!src || !dst || width <= 0 || height <= 0) {
    return;
  }
  std::copy(src, src + static_cast<std::size_t>(width) * height, dst);
  const int scanLength = std::max(2, params.scanLength);

  for (int y = 0; y < height; ++y) {
    std::vector<PixelF> line(static_cast<std::size_t>(width));
    for (int x = 0; x < width; ++x) {
      line[static_cast<std::size_t>(x)] = dst[static_cast<std::size_t>(y) * width + x];
    }
    for (int x = 0; x < width; ++x) {
      if (isMainLine(line[static_cast<std::size_t>(x)], params)) {
        dst[static_cast<std::size_t>(y) * width + x] = chooseReplacement(line, x, scanLength, params);
      }
    }
  }

  for (int x = 0; x < width; ++x) {
    std::vector<PixelF> line(static_cast<std::size_t>(height));
    for (int y = 0; y < height; ++y) {
      line[static_cast<std::size_t>(y)] = dst[static_cast<std::size_t>(y) * width + x];
    }
    for (int y = 0; y < height; ++y) {
      if (isMainLine(line[static_cast<std::size_t>(y)], params)) {
        dst[static_cast<std::size_t>(y) * width + x] = chooseReplacement(line, y, scanLength, params);
      }
    }
  }
}

void applyLineExtraction(const PixelF* src, PixelF* dst, int width, int height, const LineExtractionParams& params) {
  if (!src || !dst || width <= 0 || height <= 0) {
    return;
  }
  const int inner = std::max(0, params.innerWidth);
  const int outer = std::max(inner + 1, params.outerWidth);
  auto sampleLum = [&](int x, int y) {
    x = std::clamp(x, 0, width - 1);
    y = std::clamp(y, 0, height - 1);
    return luminance(src[static_cast<std::size_t>(y) * width + x]);
  };

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float center = sampleLum(x, y);
      float innerAvg = 0.0f;
      float outerAvg = 0.0f;
      int innerCount = 0;
      int outerCount = 0;
      for (int yy = y - outer; yy <= y + outer; ++yy) {
        for (int xx = x - outer; xx <= x + outer; ++xx) {
          const int d = std::max(std::abs(xx - x), std::abs(yy - y));
          if (d <= inner) {
            innerAvg += sampleLum(xx, yy);
            ++innerCount;
          } else if (d <= outer) {
            outerAvg += sampleLum(xx, yy);
            ++outerCount;
          }
        }
      }
      innerAvg = innerCount > 0 ? innerAvg / innerCount : center;
      outerAvg = outerCount > 0 ? outerAvg / outerCount : center;
      float edge = std::clamp(outerAvg - innerAvg, 0.0f, 1.0f);
      if (params.postLevel) {
        const float lo = std::clamp(params.levelLow, 0.0f, 1.0f);
        const float hi = std::max(lo + 0.0001f, std::clamp(params.levelHigh, 0.0f, 1.0f));
        edge = std::clamp((edge - lo) / (hi - lo), 0.0f, 1.0f);
      }
      PixelF line {params.color.r, params.color.g, params.color.b, edge};
      if (params.blend) {
        const PixelF& base = src[static_cast<std::size_t>(y) * width + x];
        line.r = base.r * (1.0f - edge) + params.color.r * edge;
        line.g = base.g * (1.0f - edge) + params.color.g * edge;
        line.b = base.b * (1.0f - edge) + params.color.b * edge;
        line.a = base.a;
      }
      dst[static_cast<std::size_t>(y) * width + x] = line;
    }
  }
}

}  // namespace selective_color_blur
