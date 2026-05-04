// SelectiveColorBlur OFX port.
// Algorithm adapted from bryful/F-s-PluginsProjects SelectiveColorBlur (MIT).

#include "SelectiveColorBlurCore.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

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
  return 0.299f * pixel.r + 0.587f * pixel.g + 0.114f * pixel.b;
}

void applyBilateral(const PixelF* src, PixelF* dst, int width, int height, int radius, float sigmaSpatial, float sigmaRange) {
  radius = std::clamp(radius, 1, 10);
  sigmaSpatial = std::max(0.01f, sigmaSpatial);
  sigmaRange = std::clamp(sigmaRange, 0.0f, 1.0f);

  const int kernelSize = radius * 2 + 1;
  std::vector<float> spatial(static_cast<std::size_t>(kernelSize * kernelSize), 0.0f);
  const float spatialDivisor = 2.0f * sigmaSpatial * sigmaSpatial + 0.00001f;
  for (int ky = -radius; ky <= radius; ++ky) {
    for (int kx = -radius; kx <= radius; ++kx) {
      spatial[static_cast<std::size_t>((ky + radius) * kernelSize + (kx + radius))] =
          std::exp(-static_cast<float>(kx * kx + ky * ky) / spatialDivisor);
    }
  }

  float rangeDivisor = 2.0f * sigmaRange * sigmaRange;
  if (rangeDivisor < 0.00001f) {
    rangeDivisor = 0.00001f;
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const PixelF& center = src[static_cast<std::size_t>(y) * width + x];
      const float centerLuma = luminance(center);
      float sumR = 0.0f;
      float sumG = 0.0f;
      float sumB = 0.0f;
      float sumW = 0.0f;

      for (int ky = -radius; ky <= radius; ++ky) {
        const int sy = std::clamp(y + ky, 0, height - 1);
        for (int kx = -radius; kx <= radius; ++kx) {
          const int sx = std::clamp(x + kx, 0, width - 1);
          const PixelF& p = src[static_cast<std::size_t>(sy) * width + sx];
          const float lumaDiff = centerLuma - luminance(p);
          float weight = spatial[static_cast<std::size_t>((ky + radius) * kernelSize + (kx + radius))];
          weight *= std::exp(-(lumaDiff * lumaDiff) / rangeDivisor);
          sumR += p.r * weight;
          sumG += p.g * weight;
          sumB += p.b * weight;
          sumW += weight;
        }
      }

      PixelF& out = dst[static_cast<std::size_t>(y) * width + x];
      if (sumW > 0.0f) {
        const float invW = 1.0f / sumW;
        out.r = sumR * invW;
        out.g = sumG * invW;
        out.b = sumB * invW;
      } else {
        out.r = center.r;
        out.g = center.g;
        out.b = center.b;
      }
      out.a = center.a;
    }
  }
}

std::vector<float> channelMinMax(const std::vector<float>& src, int width, int height, int radius, bool useMax) {
  radius = std::max(0, radius);
  if (radius <= 0 || width <= 0 || height <= 0) {
    return src;
  }

  std::vector<float> horizontal(src.size(), 0.0f);
  std::vector<float> dst(src.size(), 0.0f);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float value = useMax ? 0.0f : 1.0f;
      for (int k = -radius; k <= radius; ++k) {
        const int sx = std::clamp(x + k, 0, width - 1);
        const float sample = src[static_cast<std::size_t>(y) * width + sx];
        value = useMax ? std::max(value, sample) : std::min(value, sample);
      }
      horizontal[static_cast<std::size_t>(y) * width + x] = value;
    }
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float value = useMax ? 0.0f : 1.0f;
      for (int k = -radius; k <= radius; ++k) {
        const int sy = std::clamp(y + k, 0, height - 1);
        const float sample = horizontal[static_cast<std::size_t>(sy) * width + x];
        value = useMax ? std::max(value, sample) : std::min(value, sample);
      }
      dst[static_cast<std::size_t>(y) * width + x] = value;
    }
  }

  return dst;
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
  const std::size_t count = static_cast<std::size_t>(width) * height;
  std::vector<PixelF> normalized(count);
  for (std::size_t i = 0; i < count; ++i) {
    normalized[i] = src[i];
    if (quantize8(normalized[i].a) == 0) {
      normalized[i].r = 1.0f;
      normalized[i].g = 1.0f;
      normalized[i].b = 1.0f;
    }
  }

  std::vector<PixelF> filtered;
  const PixelF* base = normalized.data();
  if (params.bilateral) {
    filtered.resize(count);
    applyBilateral(normalized.data(), filtered.data(), width, height, params.bilateralRadius, params.bilateralSigmaSpatial, params.bilateralSigmaRange);
    base = filtered.data();
  }

  std::vector<float> gray(count, 0.0f);
  for (std::size_t i = 0; i < count; ++i) {
    const PixelF& p = base[i];
    const float mx = std::max({p.r, p.g, p.b});
    const float mn = std::min({p.r, p.g, p.b});
    gray[i] = std::clamp(p.a * (mx + mn) * 0.5f, 0.0f, 1.0f);
  }

  const int outer = std::clamp(params.outerWidth, 0, 100);
  const int inner = std::clamp(params.innerWidth, 0, 100);
  std::vector<float> red = gray;
  std::vector<float> green = gray;
  if (outer > 0) {
    red = channelMinMax(gray, width, height, outer, true);
  }
  if (inner > 0) {
    green = channelMinMax(gray, width, height, inner, false);
  }

  if (outer <= 0 && inner <= 0) {
    for (std::size_t i = 0; i < count; ++i) {
      const float value = gray[i];
      dst[i] = PixelF {value, value, value, 1.0f};
    }
    return;
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const std::size_t index = static_cast<std::size_t>(y) * width + x;
      float edge = std::clamp(std::abs(red[index] - green[index]), 0.0f, 1.0f);
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
      dst[index] = line;
    }
  }
}

struct ThinPixel8 {
  int alpha = 0;
  int red = 0;
  int green = 0;
  int blue = 0;
};

struct ThinPixel16 {
  int alpha = 0;
  int red = 0;
  int green = 0;
  int blue = 0;
};

struct ThinPixel {
  float alpha = 0.0f;
  float red = 0.0f;
  float green = 0.0f;
  float blue = 0.0f;
};

struct ThinInfo;
struct ThinBak {
  bool drawFlag = false;
  ThinPixel color;
  int dir = -1;
};

struct ThinInfo {
  int w = 0;
  int wt = 0;
  int wt2 = 0;
  int h = 0;
  std::vector<ThinPixel> scanline;
  ThinPixel* data = nullptr;
  bool white = false;
  bool alphaZero = false;
  bool edge = false;
  int target_color_count = 0;
  ThinPixel8 color8[8];
  int level = 0;
  int nowX = 0;
  int nowY = 0;
};

template <typename PixelType>
struct PixelTraits;

template <>
struct PixelTraits<ThinPixel> {
  using InfoType = ThinInfo;
  using BakType = ThinBak;

  static ThinPixel ConvertFrom8bit(const ThinPixel8& p) {
    return {static_cast<float>(p.alpha) / 255.0f,
            static_cast<float>(p.red) / 255.0f,
            static_cast<float>(p.green) / 255.0f,
            static_cast<float>(p.blue) / 255.0f};
  }

  static ThinPixel8 ConvertTo8bit(const ThinPixel& p) {
    return {quantize8(p.alpha), quantize8(p.red), quantize8(p.green), quantize8(p.blue)};
  }
};

#include "ThinPattern.inc"

ThinPixel toThinPixel(const PixelF& pixel) {
  if (quantize8(pixel.a) == 0) {
    return {0.0f, 1.0f, 1.0f, 1.0f};
  }
  return {pixel.a, pixel.r, pixel.g, pixel.b};
}

PixelF fromThinPixel(const ThinPixel& pixel) {
  return {pixel.red, pixel.green, pixel.blue, pixel.alpha};
}

ThinPixel8 toThinTarget(const Color& color) {
  return {quantize8(color.a), quantize8(color.r), quantize8(color.g), quantize8(color.b)};
}

void runThinPass(ThinInfo& ti, ThinBak (*pattern)(ThinInfo*)) {
  int now = 0;
  for (int y = 0; y < ti.h; ++y) {
    ti.nowY = y;
    scanlineCopy<ThinPixel>(&ti, y);
    for (int x = 0; x < ti.w; ++x) {
      ti.nowX = x;
      ThinPixel p = getScanLine<ThinPixel>(&ti, 0, 0);
      if (CompPx<ThinPixel>(&ti, p)) {
        ThinBak tb = pattern(&ti);
        if (tb.drawFlag && !shouldSkipPixel<ThinPixel>(&ti, tb.color)) {
          ti.data[now] = tb.color;
        }
      }
      ++now;
    }
  }
}

void applyThin(const PixelF* src, PixelF* dst, int width, int height, const ThinParams& params) {
  if (!src || !dst || width <= 0 || height <= 0) {
    return;
  }
  std::copy(src, src + static_cast<std::size_t>(width) * height, dst);
  const int iterations = std::clamp(params.thinValue, 0, 16);
  if (iterations <= 0) {
    return;
  }

  std::array<Color, 4> enabledColors {};
  int enabledColorCount = 0;
  for (const TargetColor& target : params.targets) {
    if (!target.enabled) {
      continue;
    }
    const auto r = quantize8(target.color.r);
    const auto g = quantize8(target.color.g);
    const auto b = quantize8(target.color.b);
    bool duplicate = false;
    for (int i = 0; i < enabledColorCount; ++i) {
      duplicate = quantize8(enabledColors[static_cast<std::size_t>(i)].r) == r &&
                  quantize8(enabledColors[static_cast<std::size_t>(i)].g) == g &&
                  quantize8(enabledColors[static_cast<std::size_t>(i)].b) == b;
      if (duplicate) {
        break;
      }
    }
    if (!duplicate && enabledColorCount < static_cast<int>(enabledColors.size())) {
      enabledColors[static_cast<std::size_t>(enabledColorCount)] = target.color;
      ++enabledColorCount;
    }
  }
  if (enabledColorCount <= 0) {
    return;
  }

  std::vector<ThinPixel> data(static_cast<std::size_t>(width) * height);
  for (std::size_t i = 0; i < data.size(); ++i) {
    data[i] = toThinPixel(src[i]);
  }

  ThinInfo ti;
  ti.target_color_count = enabledColorCount;
  for (int i = 0; i < ti.target_color_count; ++i) {
    ti.color8[i] = toThinTarget(enabledColors[static_cast<std::size_t>(i)]);
  }
  ti.level = std::clamp(static_cast<int>(std::lround(params.targetLevel * 255.0f)), 0, 255);
  ti.w = width;
  ti.wt = width;
  ti.wt2 = width * 2;
  ti.h = height;
  ti.data = data.data();
  ti.white = params.ignoreWhite;
  ti.alphaZero = params.ignoreTransparent;
  ti.edge = params.refineEdges;
  ti.scanline.resize(static_cast<std::size_t>(width) * 3U);

  for (int pass = 0; pass < iterations; ++pass) {
    if (ti.edge) {
      runThinPass(ti, getPatEdge<ThinPixel>);
    }
    runThinPass(ti, getPat<ThinPixel>);
    runThinPass(ti, getPatDot<ThinPixel>);

    int now = 0;
    for (int y = 0; y < ti.h; ++y) {
      ti.nowY = y;
      scanlineCopy<ThinPixel>(&ti, y);
      for (int x = 0; x < ti.w; ++x) {
        ti.nowX = x;
        int targetIndex = now;
        ThinPixel p = getScanLine<ThinPixel>(&ti, 0, 0);
        if (CompPx<ThinPixel>(&ti, p)) {
          ThinBak tb = getPatDot2nd<ThinPixel>(&ti);
          if (tb.drawFlag) {
            switch (tb.dir) {
              case 0: tb.drawFlag = ti.nowY > 0; targetIndex = now - ti.wt; break;
              case 1: tb.drawFlag = ti.nowX < (ti.w - 1); targetIndex = now + 1; break;
              case 2: tb.drawFlag = ti.nowY < (ti.h - 1); targetIndex = now + ti.wt; break;
              case 3: tb.drawFlag = ti.nowX > 0; targetIndex = now - 1; break;
              default: tb.drawFlag = false; break;
            }
          }
          if (tb.drawFlag && !shouldSkipPixel<ThinPixel>(&ti, ti.data[targetIndex])) {
            ti.data[targetIndex] = tb.color;
          }
        }
        ++now;
      }
    }
  }

  for (std::size_t i = 0; i < data.size(); ++i) {
    dst[i] = fromThinPixel(data[i]);
  }
}

}  // namespace selective_color_blur
