// SelectiveColorBlur OFX port.
// Algorithm adapted from bryful/F-s-PluginsProjects SelectiveColorBlur (MIT).

#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace selective_color_blur {

struct Color {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 1.0f;
};

struct TargetColor {
  bool enabled = false;
  Color color;
};

struct Parameters {
  int radius = 0;
  std::array<TargetColor, 8> targets;
};

struct PixelF {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 0.0f;
};

void apply(const PixelF* src, PixelF* dst, int width, int height, const Parameters& params);

std::uint8_t quantize8(float value);

bool matchesColor(const PixelF& pixel, const Color& color, int tolerance8);
float luminance(const PixelF& pixel);

struct SelectColorParams {
  bool reverse = false;
  int tolerance8 = 0;
  std::array<TargetColor, 8> targets;
};

struct ColorSwitchEntry {
  bool enabled = false;
  Color oldColor;
  Color newColor;
};

enum class ColorSwitchMode {
  Replace = 1,
  Key = 2,
  Extract = 3,
};

struct ColorSwitchParams {
  bool enableAll = true;
  int activeCount = 8;
  ColorSwitchMode mode = ColorSwitchMode::Replace;
  std::array<ColorSwitchEntry, 32> entries;
};

struct EdgeLineParams {
  Color targetColor {1.0f, 0.0f, 0.0f, 1.0f};
  Color sampleColor {0.0f, 1.0f, 0.0f, 1.0f};
  int tolerance8 = 0;
  int length = 10;
  Color drawColor {0.0f, 0.0f, 1.0f, 1.0f};
};

enum class RimFillMode {
  CustomColor = 1,
  AdjacentColor = 2,
};

struct RimFillParams {
  int width = 0;
  RimFillMode mode = RimFillMode::CustomColor;
  Color customColor {1.0f, 1.0f, 1.0f, 1.0f};
  bool whiteIsTransparent = false;
};

struct MainLineRepaintParams {
  Color mainColor {0.0f, 0.0f, 0.0f, 1.0f};
  int tolerance8 = 0;
  int scanLength = 16;
};

struct LineExtractionParams {
  bool bilateral = true;
  int bilateralRadius = 2;
  float bilateralSigmaSpatial = 1.0f;
  float bilateralSigmaRange = 0.15f;
  int innerWidth = 1;
  int outerWidth = 1;
  Color color {1.0f, 1.0f, 1.0f, 1.0f};
  bool postLevel = true;
  float levelLow = 0.05f;
  float levelHigh = 0.9f;
  bool blend = false;
};

struct ThinParams {
  float targetLevel = 0.0f;
  std::array<TargetColor, 4> targets;
  int thinValue = 0;
  bool ignoreWhite = false;
  bool ignoreTransparent = false;
  bool refineEdges = false;
};

void applySelectColor(const PixelF* src, PixelF* dst, int width, int height, const SelectColorParams& params);
void applyColorSwitch(const PixelF* src, PixelF* dst, int width, int height, const ColorSwitchParams& params);
void applyEdgeLine(const PixelF* src, PixelF* dst, int width, int height, const EdgeLineParams& params);
void applyRimFill(const PixelF* src, PixelF* dst, int width, int height, const RimFillParams& params);
void applyMainLineRepaint(const PixelF* src, PixelF* dst, int width, int height, const MainLineRepaintParams& params);
void applyLineExtraction(const PixelF* src, PixelF* dst, int width, int height, const LineExtractionParams& params);
void applyThin(const PixelF* src, PixelF* dst, int width, int height, const ThinParams& params);

}  // namespace selective_color_blur
