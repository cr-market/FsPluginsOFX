#include "SelectiveColorBlurCore.h"

#include <cassert>
#include <cmath>
#include <vector>

using selective_color_blur::Parameters;
using selective_color_blur::PixelF;

namespace {

bool near(float a, float b) {
  return std::fabs(a - b) < 0.0001f;
}

}  // namespace

int main() {
  std::vector<PixelF> src = {
      {1.0f, 0.0f, 0.0f, 1.0f},
      {0.0f, 1.0f, 0.0f, 1.0f},
      {1.0f, 0.0f, 0.0f, 1.0f},
      {0.0f, 0.0f, 1.0f, 1.0f},
      {1.0f, 0.0f, 0.0f, 1.0f},
      {0.0f, 0.0f, 1.0f, 1.0f},
  };
  std::vector<PixelF> dst(src.size());

  Parameters identity;
  identity.radius = 8;
  selective_color_blur::apply(src.data(), dst.data(), 3, 2, identity);
  assert(near(dst[1].g, 1.0f));
  assert(near(dst[3].b, 1.0f));

  Parameters params;
  params.radius = 2;
  params.targets[0].enabled = true;
  params.targets[0].color = {1.0f, 0.0f, 0.0f, 1.0f};
  selective_color_blur::apply(src.data(), dst.data(), 3, 2, params);

  assert(near(dst[1].g, 1.0f));
  assert(near(dst[3].b, 1.0f));
  assert(near(dst[0].a, 1.0f));
  assert(near(dst[2].a, 1.0f));

  assert(selective_color_blur::quantize8(1.0f) == 255);
  assert(selective_color_blur::quantize8(0.0f) == 0);
  assert(selective_color_blur::quantize8(0.5f) == 128);

  selective_color_blur::SelectColorParams select;
  select.targets[0].enabled = true;
  select.targets[0].color = {1.0f, 0.0f, 0.0f, 1.0f};
  selective_color_blur::applySelectColor(src.data(), dst.data(), 3, 2, select);
  assert(near(dst[0].a, 1.0f));
  assert(near(dst[1].a, 0.0f));

  selective_color_blur::ColorSwitchParams sw;
  sw.entries[0].enabled = true;
  sw.entries[0].oldColor = {1.0f, 0.0f, 0.0f, 1.0f};
  sw.entries[0].newColor = {0.0f, 0.0f, 0.0f, 1.0f};
  selective_color_blur::applyColorSwitch(src.data(), dst.data(), 3, 2, sw);
  assert(near(dst[0].r, 0.0f));
  assert(near(dst[1].g, 1.0f));

  selective_color_blur::EdgeLineParams edge;
  edge.length = 1;
  edge.drawColor = {0.25f, 0.25f, 0.25f, 1.0f};
  selective_color_blur::applyEdgeLine(src.data(), dst.data(), 3, 2, edge);
  assert(near(dst[0].r, 0.25f));

  std::vector<PixelF> rim = {
      {0.0f, 0.0f, 0.0f, 0.0f},
      {0.8f, 0.1f, 0.1f, 1.0f},
      {0.0f, 0.0f, 0.0f, 0.0f},
  };
  selective_color_blur::RimFillParams rimParams;
  rimParams.width = 1;
  rimParams.mode = selective_color_blur::RimFillMode::AdjacentColor;
  selective_color_blur::applyRimFill(rim.data(), dst.data(), 3, 1, rimParams);
  assert(near(dst[0].r, 0.8f));
  assert(near(dst[2].r, 0.8f));

  selective_color_blur::MainLineRepaintParams repaint;
  repaint.mainColor = {1.0f, 0.0f, 0.0f, 1.0f};
  selective_color_blur::applyMainLineRepaint(src.data(), dst.data(), 3, 2, repaint);
  assert(near(dst[0].r, 0.0f) || near(dst[0].b, 1.0f));

  selective_color_blur::LineExtractionParams line;
  line.outerWidth = 1;
  line.color = {0.0f, 0.0f, 0.0f, 1.0f};
  selective_color_blur::applyLineExtraction(src.data(), dst.data(), 3, 2, line);
  assert(dst[0].a >= 0.0f && dst[0].a <= 1.0f);

  std::vector<PixelF> noisy = {
      {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f},
  };
  selective_color_blur::LineExtractionParams noBilateral;
  noBilateral.bilateral = false;
  noBilateral.innerWidth = 0;
  noBilateral.outerWidth = 1;
  selective_color_blur::LineExtractionParams withBilateral = noBilateral;
  withBilateral.bilateral = true;
  withBilateral.bilateralRadius = 1;
  withBilateral.bilateralSigmaSpatial = 4.0f;
  withBilateral.bilateralSigmaRange = 1.0f;
  selective_color_blur::applyLineExtraction(noisy.data(), dst.data(), 3, 1, noBilateral);
  const float rawAlpha0 = dst[0].a;
  const float rawAlpha1 = dst[1].a;
  const float rawAlpha2 = dst[2].a;
  selective_color_blur::applyLineExtraction(noisy.data(), dst.data(), 3, 1, withBilateral);
  const float diff = std::abs(dst[0].a - rawAlpha0) + std::abs(dst[1].a - rawAlpha1) + std::abs(dst[2].a - rawAlpha2);
  assert(diff > 0.0001f);

  std::vector<PixelF> thinSrc = {
      {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
  };
  selective_color_blur::ThinParams thin;
  thin.targetColorCount = 1;
  thin.targetColors[0] = {0.0f, 0.0f, 0.0f, 1.0f};
  thin.thinValue = 1;
  selective_color_blur::applyThin(thinSrc.data(), dst.data(), 3, 1, thin);
  assert(dst[1].r > 0.5f);
  return 0;
}
