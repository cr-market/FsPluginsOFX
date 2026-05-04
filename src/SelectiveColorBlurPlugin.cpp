// F's/NF Plugins OFX port bundle.
// Algorithms adapted from bryful/F-s-PluginsProjects (MIT).

#include "SelectiveColorBlurCore.h"

#include "ofxsImageEffect.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace {

using namespace selective_color_blur;

constexpr int kTargetCount = 8;
constexpr int kColorSwitchCount = 32;
constexpr const char* kPluginGrouping = "F's Plugins OFX";

enum class EffectKind {
  SelectiveColorBlur,
  MainLineRepaint,
  SelectColor,
  ColorSwitch,
  EdgeLine,
  RimFill,
  LineExtraction,
};

struct EffectSpec {
  EffectKind kind;
  const char* id;
  const char* label;
  int major;
  int minor;
};

constexpr EffectSpec kSelectiveColorBlur {EffectKind::SelectiveColorBlur, "com.codex.fsplugins.SelectiveColorBlur", "FS-SelectiveColorBlur", 1, 0};
constexpr EffectSpec kMainLineRepaint {EffectKind::MainLineRepaint, "com.codex.fsplugins.MainLineRepaint", "FS-MainLineRepaint", 1, 0};
constexpr EffectSpec kSelectColor {EffectKind::SelectColor, "com.codex.fsplugins.SelectColor", "FS-SelectColor", 1, 0};
constexpr EffectSpec kColorSwitch {EffectKind::ColorSwitch, "com.codex.fsplugins.ColorSwitch", "FS-ColorSwitch", 1, 0};
constexpr EffectSpec kEdgeLine {EffectKind::EdgeLine, "com.codex.fsplugins.EdgeLine", "FS-EdgeLine", 1, 0};
constexpr EffectSpec kRimFill {EffectKind::RimFill, "com.codex.fsplugins.RimFill", "FS-RimFill", 1, 0};
constexpr EffectSpec kLineExtraction {EffectKind::LineExtraction, "com.codex.fsplugins.LineExtraction", "FS-LineExtraction", 1, 0};

const std::array<Color, kTargetCount> kDefaultTargetColors = {{
    {1.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 1.0f, 1.0f},
    {1.0f, 0.0f, 1.0f, 1.0f},
    {0.5f, 0.5f, 0.5f, 1.0f},
    {0.875f, 0.875f, 0.875f, 1.0f},
}};

std::string targetEnabledName(int index) {
  return "target" + std::to_string(index) + "Enabled";
}

std::string targetColorName(int index) {
  return "target" + std::to_string(index) + "Color";
}

std::string colorSwitchEnabledName(int index) {
  return "switch" + std::to_string(index) + "Enabled";
}

std::string colorSwitchOldName(int index) {
  return "switch" + std::to_string(index) + "Old";
}

std::string colorSwitchNewName(int index) {
  return "switch" + std::to_string(index) + "New";
}

Color getColor(OFX::RGBAParam* param, double time) {
  double r = 0.0;
  double g = 0.0;
  double b = 0.0;
  double a = 1.0;
  param->getValueAtTime(time, r, g, b, a);
  return {static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), static_cast<float>(a)};
}

template <typename Channel>
float toFloat(Channel value, float maxValue) {
  return static_cast<float>(value) / maxValue;
}

template <typename Channel>
Channel fromFloat(float value, float maxValue) {
  const float clamped = std::clamp(value, 0.0f, 1.0f);
  return static_cast<Channel>(clamped * maxValue + 0.5f);
}

template <>
float fromFloat<float>(float value, float /*maxValue*/) {
  return value;
}

template <typename Channel>
void readImage(const OFX::Image& image, const OfxRectI& bounds, std::vector<PixelF>& pixels, float maxValue) {
  const int width = bounds.x2 - bounds.x1;
  const int height = bounds.y2 - bounds.y1;
  pixels.assign(static_cast<std::size_t>(width) * height, {});
  for (int y = bounds.y1; y < bounds.y2; ++y) {
    for (int x = bounds.x1; x < bounds.x2; ++x) {
      const Channel* src = static_cast<const Channel*>(image.getPixelAddress(x, y));
      PixelF& out = pixels[static_cast<std::size_t>(y - bounds.y1) * width + (x - bounds.x1)];
      if (!src) {
        continue;
      }
      out.r = toFloat(src[0], maxValue);
      out.g = toFloat(src[1], maxValue);
      out.b = toFloat(src[2], maxValue);
      out.a = toFloat(src[3], maxValue);
    }
  }
}

template <typename Channel>
void writeImage(OFX::Image& image,
                const OfxRectI& srcBounds,
                const OfxRectI& renderWindow,
                const std::vector<PixelF>& pixels,
                float maxValue) {
  const int width = srcBounds.x2 - srcBounds.x1;
  const int x1 = std::max(renderWindow.x1, srcBounds.x1);
  const int y1 = std::max(renderWindow.y1, srcBounds.y1);
  const int x2 = std::min(renderWindow.x2, srcBounds.x2);
  const int y2 = std::min(renderWindow.y2, srcBounds.y2);

  for (int y = y1; y < y2; ++y) {
    for (int x = x1; x < x2; ++x) {
      Channel* dst = static_cast<Channel*>(image.getPixelAddress(x, y));
      if (!dst) {
        continue;
      }
      const PixelF& src = pixels[static_cast<std::size_t>(y - srcBounds.y1) * width + (x - srcBounds.x1)];
      dst[0] = fromFloat<Channel>(src.r, maxValue);
      dst[1] = fromFloat<Channel>(src.g, maxValue);
      dst[2] = fromFloat<Channel>(src.b, maxValue);
      dst[3] = fromFloat<Channel>(src.a, maxValue);
    }
  }
}

void setCommonDescriptor(OFX::ImageEffectDescriptor& desc, const EffectSpec& spec) {
  desc.setLabels(spec.label, spec.label, spec.label);
  desc.setPluginGrouping(kPluginGrouping);
  desc.addSupportedContext(OFX::eContextFilter);
  desc.addSupportedBitDepth(OFX::eBitDepthUByte);
  desc.addSupportedBitDepth(OFX::eBitDepthUShort);
  desc.addSupportedBitDepth(OFX::eBitDepthFloat);
  desc.setSingleInstance(false);
  desc.setHostFrameThreading(false);
  desc.setSupportsMultiResolution(true);
  desc.setSupportsTiles(false);
  desc.setTemporalClipAccess(false);
  desc.setRenderTwiceAlways(false);
  desc.setSupportsMultipleClipPARs(false);
}

void defineClips(OFX::ImageEffectDescriptor& desc) {
  OFX::ClipDescriptor* srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
  srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
  srcClip->setTemporalClipAccess(false);
  srcClip->setSupportsTiles(false);

  OFX::ClipDescriptor* dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
  dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
  dstClip->setSupportsTiles(false);
}

OFX::IntParamDescriptor* defineInt(OFX::ImageEffectDescriptor& desc,
                                   OFX::PageParamDescriptor& page,
                                   const std::string& name,
                                   const std::string& label,
                                   int def,
                                   int min,
                                   int max,
                                   int displayMax,
                                   OFX::GroupParamDescriptor* parent = nullptr) {
  OFX::IntParamDescriptor* param = desc.defineIntParam(name);
  param->setLabels(label, label, label);
  param->setScriptName(name);
  param->setDefault(def);
  param->setRange(min, max);
  param->setDisplayRange(min, displayMax);
  if (parent) {
    param->setParent(*parent);
  }
  page.addChild(*param);
  return param;
}

OFX::DoubleParamDescriptor* defineDouble(OFX::ImageEffectDescriptor& desc,
                                         OFX::PageParamDescriptor& page,
                                         const std::string& name,
                                         const std::string& label,
                                         double def,
                                         double min,
                                         double max,
                                         OFX::GroupParamDescriptor* parent = nullptr) {
  OFX::DoubleParamDescriptor* param = desc.defineDoubleParam(name);
  param->setLabels(label, label, label);
  param->setScriptName(name);
  param->setDefault(def);
  param->setRange(min, max);
  param->setDisplayRange(min, max);
  if (parent) {
    param->setParent(*parent);
  }
  page.addChild(*param);
  return param;
}

OFX::BooleanParamDescriptor* defineBool(OFX::ImageEffectDescriptor& desc,
                                        OFX::PageParamDescriptor& page,
                                        const std::string& name,
                                        const std::string& label,
                                        bool def,
                                        OFX::GroupParamDescriptor* parent = nullptr) {
  OFX::BooleanParamDescriptor* param = desc.defineBooleanParam(name);
  param->setLabels(label, label, label);
  param->setScriptName(name);
  param->setDefault(def);
  if (parent) {
    param->setParent(*parent);
  }
  page.addChild(*param);
  return param;
}

OFX::RGBAParamDescriptor* defineColor(OFX::ImageEffectDescriptor& desc,
                                      OFX::PageParamDescriptor& page,
                                      const std::string& name,
                                      const std::string& label,
                                      Color def,
                                      OFX::GroupParamDescriptor* parent = nullptr) {
  OFX::RGBAParamDescriptor* param = desc.defineRGBAParam(name);
  param->setLabels(label, label, label);
  param->setScriptName(name);
  param->setDefault(def.r, def.g, def.b, def.a);
  if (parent) {
    param->setParent(*parent);
  }
  page.addChild(*param);
  return param;
}

OFX::ChoiceParamDescriptor* defineChoice(OFX::ImageEffectDescriptor& desc,
                                         OFX::PageParamDescriptor& page,
                                         const std::string& name,
                                         const std::string& label,
                                         int def,
                                         const std::vector<std::string>& options) {
  OFX::ChoiceParamDescriptor* param = desc.defineChoiceParam(name);
  param->setLabels(label, label, label);
  param->setScriptName(name);
  for (const std::string& option : options) {
    param->appendOption(option, option);
  }
  param->setDefault(def);
  page.addChild(*param);
  return param;
}

void defineTargetColorGroup(OFX::ImageEffectDescriptor& desc, OFX::PageParamDescriptor& page) {
  OFX::GroupParamDescriptor* group = desc.defineGroupParam("targetColors");
  group->setLabels("Target Colors", "Target Colors", "Target Colors");
  page.addChild(*group);
  for (int i = 0; i < kTargetCount; ++i) {
    defineBool(desc, page, targetEnabledName(i), "Target " + std::to_string(i) + " On", false, group);
    defineColor(desc, page, targetColorName(i), "Target " + std::to_string(i) + " Color", kDefaultTargetColors[static_cast<std::size_t>(i)], group);
  }
}

class FsPlugin final : public OFX::ImageEffect {
 public:
  FsPlugin(OfxImageEffectHandle handle, EffectKind kind)
      : OFX::ImageEffect(handle),
        kind_(kind),
        dstClip_(this->fetchClip(kOfxImageEffectOutputClipName)),
        srcClip_(this->fetchClip(kOfxImageEffectSimpleSourceClipName)) {
    switch (kind_) {
      case EffectKind::SelectiveColorBlur:
        blur_ = this->fetchIntParam("blur");
        fetchTargetParams();
        break;
      case EffectKind::MainLineRepaint:
        mainColor_ = this->fetchRGBAParam("mainColor");
        tolerance_ = this->fetchIntParam("tolerance");
        scanLength_ = this->fetchIntParam("scanLength");
        break;
      case EffectKind::SelectColor:
        reverse_ = this->fetchBooleanParam("reverse");
        tolerance_ = this->fetchIntParam("tolerance");
        fetchTargetParams();
        break;
      case EffectKind::ColorSwitch:
        enableAll_ = this->fetchBooleanParam("enableAll");
        activeCount_ = this->fetchIntParam("activeCount");
        mode_ = this->fetchChoiceParam("mode");
        for (int i = 0; i < kColorSwitchCount; ++i) {
          switchEnabled_[static_cast<std::size_t>(i)] = this->fetchBooleanParam(colorSwitchEnabledName(i));
          switchOld_[static_cast<std::size_t>(i)] = this->fetchRGBAParam(colorSwitchOldName(i));
          switchNew_[static_cast<std::size_t>(i)] = this->fetchRGBAParam(colorSwitchNewName(i));
        }
        break;
      case EffectKind::EdgeLine:
        targetColor_ = this->fetchRGBAParam("targetColor");
        sampleColor_ = this->fetchRGBAParam("sampleColor");
        tolerance_ = this->fetchIntParam("tolerance");
        length_ = this->fetchIntParam("length");
        drawColor_ = this->fetchRGBAParam("drawColor");
        break;
      case EffectKind::RimFill:
        width_ = this->fetchIntParam("width");
        mode_ = this->fetchChoiceParam("mode");
        customColor_ = this->fetchRGBAParam("customColor");
        whiteTransparent_ = this->fetchBooleanParam("whiteTransparent");
        break;
      case EffectKind::LineExtraction:
        innerWidth_ = this->fetchIntParam("innerWidth");
        outerWidth_ = this->fetchIntParam("outerWidth");
        drawColor_ = this->fetchRGBAParam("drawColor");
        postLevel_ = this->fetchBooleanParam("postLevel");
        levelLow_ = this->fetchDoubleParam("levelLow");
        levelHigh_ = this->fetchDoubleParam("levelHigh");
        blend_ = this->fetchBooleanParam("blend");
        break;
    }
  }

  void render(const OFX::RenderArguments& args) override {
    std::unique_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));
    std::unique_ptr<OFX::Image> src(srcClip_->fetchImage(args.time));
    if (!dst || !src) {
      OFX::throwSuiteStatusException(kOfxStatFailed);
    }
    if (src->getPixelComponents() != OFX::ePixelComponentRGBA ||
        dst->getPixelComponents() != OFX::ePixelComponentRGBA ||
        src->getPixelDepth() != dst->getPixelDepth()) {
      OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }

    const OfxRectI& bounds = src->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;
    if (width <= 0 || height <= 0) {
      return;
    }

    std::vector<PixelF> input;
    std::vector<PixelF> output;
    switch (src->getPixelDepth()) {
      case OFX::eBitDepthUByte:
        readImage<unsigned char>(*src, bounds, input, 255.0f);
        output.resize(input.size());
        process(input, output, width, height, args.time);
        writeImage<unsigned char>(*dst, bounds, args.renderWindow, output, 255.0f);
        break;
      case OFX::eBitDepthUShort:
        readImage<unsigned short>(*src, bounds, input, 65535.0f);
        output.resize(input.size());
        process(input, output, width, height, args.time);
        writeImage<unsigned short>(*dst, bounds, args.renderWindow, output, 65535.0f);
        break;
      case OFX::eBitDepthFloat:
        readImage<float>(*src, bounds, input, 1.0f);
        output.resize(input.size());
        process(input, output, width, height, args.time);
        writeImage<float>(*dst, bounds, args.renderWindow, output, 1.0f);
        break;
      default:
        OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  }

  bool isIdentity(const OFX::IsIdentityArguments& args, OFX::Clip*& identityClip, double& identityTime) override {
    bool identity = false;
    if (kind_ == EffectKind::SelectiveColorBlur) {
      identity = blur_->getValueAtTime(args.time) <= 0 || !hasEnabledTarget(args.time);
    } else if (kind_ == EffectKind::SelectColor) {
      identity = !hasEnabledTarget(args.time);
    } else if (kind_ == EffectKind::RimFill) {
      identity = width_->getValueAtTime(args.time) <= 0;
    } else if (kind_ == EffectKind::EdgeLine) {
      identity = length_->getValueAtTime(args.time) <= 0;
    }
    if (identity) {
      identityClip = srcClip_;
      identityTime = args.time;
      return true;
    }
    return false;
  }

  bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments& args, OfxRectD& rod) override {
    rod = srcClip_->getRegionOfDefinition(args.time);
    return true;
  }

  void getRegionsOfInterest(const OFX::RegionsOfInterestArguments& args, OFX::RegionOfInterestSetter& rois) override {
    rois.setRegionOfInterest(*srcClip_, args.regionOfInterest);
  }

 private:
  void fetchTargetParams() {
    for (int i = 0; i < kTargetCount; ++i) {
      targetEnabled_[static_cast<std::size_t>(i)] = this->fetchBooleanParam(targetEnabledName(i));
      targetColors_[static_cast<std::size_t>(i)] = this->fetchRGBAParam(targetColorName(i));
    }
  }

  bool hasEnabledTarget(double time) const {
    for (OFX::BooleanParam* param : targetEnabled_) {
      if (param && param->getValueAtTime(time)) {
        return true;
      }
    }
    return false;
  }

  std::array<TargetColor, kTargetCount> fetchTargets(double time) const {
    std::array<TargetColor, kTargetCount> targets;
    for (int i = 0; i < kTargetCount; ++i) {
      targets[static_cast<std::size_t>(i)].enabled = targetEnabled_[static_cast<std::size_t>(i)]->getValueAtTime(time);
      targets[static_cast<std::size_t>(i)].color = getColor(targetColors_[static_cast<std::size_t>(i)], time);
    }
    return targets;
  }

  void process(const std::vector<PixelF>& input, std::vector<PixelF>& output, int width, int height, double time) const {
    switch (kind_) {
      case EffectKind::SelectiveColorBlur: {
        Parameters params;
        params.radius = blur_->getValueAtTime(time);
        params.targets = fetchTargets(time);
        apply(input.data(), output.data(), width, height, params);
        break;
      }
      case EffectKind::MainLineRepaint: {
        MainLineRepaintParams params;
        params.mainColor = getColor(mainColor_, time);
        params.tolerance8 = tolerance_->getValueAtTime(time);
        params.scanLength = scanLength_->getValueAtTime(time);
        applyMainLineRepaint(input.data(), output.data(), width, height, params);
        break;
      }
      case EffectKind::SelectColor: {
        SelectColorParams params;
        params.reverse = reverse_->getValueAtTime(time);
        params.tolerance8 = tolerance_->getValueAtTime(time);
        params.targets = fetchTargets(time);
        applySelectColor(input.data(), output.data(), width, height, params);
        break;
      }
      case EffectKind::ColorSwitch: {
        ColorSwitchParams params;
        params.enableAll = enableAll_->getValueAtTime(time);
        params.activeCount = activeCount_->getValueAtTime(time);
        int mode = 0;
        mode_->getValueAtTime(time, mode);
        params.mode = static_cast<ColorSwitchMode>(mode + 1);
        for (int i = 0; i < kColorSwitchCount; ++i) {
          ColorSwitchEntry& entry = params.entries[static_cast<std::size_t>(i)];
          entry.enabled = switchEnabled_[static_cast<std::size_t>(i)]->getValueAtTime(time);
          entry.oldColor = getColor(switchOld_[static_cast<std::size_t>(i)], time);
          entry.newColor = getColor(switchNew_[static_cast<std::size_t>(i)], time);
        }
        applyColorSwitch(input.data(), output.data(), width, height, params);
        break;
      }
      case EffectKind::EdgeLine: {
        EdgeLineParams params;
        params.targetColor = getColor(targetColor_, time);
        params.sampleColor = getColor(sampleColor_, time);
        params.tolerance8 = tolerance_->getValueAtTime(time);
        params.length = length_->getValueAtTime(time);
        params.drawColor = getColor(drawColor_, time);
        applyEdgeLine(input.data(), output.data(), width, height, params);
        break;
      }
      case EffectKind::RimFill: {
        RimFillParams params;
        params.width = width_->getValueAtTime(time);
        int mode = 0;
        mode_->getValueAtTime(time, mode);
        params.mode = mode == 0 ? RimFillMode::CustomColor : RimFillMode::AdjacentColor;
        params.customColor = getColor(customColor_, time);
        params.whiteIsTransparent = whiteTransparent_->getValueAtTime(time);
        applyRimFill(input.data(), output.data(), width, height, params);
        break;
      }
      case EffectKind::LineExtraction: {
        LineExtractionParams params;
        params.innerWidth = innerWidth_->getValueAtTime(time);
        params.outerWidth = outerWidth_->getValueAtTime(time);
        params.color = getColor(drawColor_, time);
        params.postLevel = postLevel_->getValueAtTime(time);
        params.levelLow = static_cast<float>(levelLow_->getValueAtTime(time));
        params.levelHigh = static_cast<float>(levelHigh_->getValueAtTime(time));
        params.blend = blend_->getValueAtTime(time);
        applyLineExtraction(input.data(), output.data(), width, height, params);
        break;
      }
    }
  }

  EffectKind kind_;
  OFX::Clip* dstClip_;
  OFX::Clip* srcClip_;

  OFX::IntParam* blur_ = nullptr;
  OFX::RGBAParam* mainColor_ = nullptr;
  OFX::IntParam* tolerance_ = nullptr;
  OFX::IntParam* scanLength_ = nullptr;
  OFX::BooleanParam* reverse_ = nullptr;
  OFX::BooleanParam* enableAll_ = nullptr;
  OFX::IntParam* activeCount_ = nullptr;
  OFX::ChoiceParam* mode_ = nullptr;
  OFX::RGBAParam* targetColor_ = nullptr;
  OFX::RGBAParam* sampleColor_ = nullptr;
  OFX::IntParam* length_ = nullptr;
  OFX::RGBAParam* drawColor_ = nullptr;
  OFX::IntParam* width_ = nullptr;
  OFX::RGBAParam* customColor_ = nullptr;
  OFX::BooleanParam* whiteTransparent_ = nullptr;
  OFX::IntParam* innerWidth_ = nullptr;
  OFX::IntParam* outerWidth_ = nullptr;
  OFX::BooleanParam* postLevel_ = nullptr;
  OFX::DoubleParam* levelLow_ = nullptr;
  OFX::DoubleParam* levelHigh_ = nullptr;
  OFX::BooleanParam* blend_ = nullptr;

  std::array<OFX::BooleanParam*, kTargetCount> targetEnabled_{};
  std::array<OFX::RGBAParam*, kTargetCount> targetColors_{};
  std::array<OFX::BooleanParam*, kColorSwitchCount> switchEnabled_{};
  std::array<OFX::RGBAParam*, kColorSwitchCount> switchOld_{};
  std::array<OFX::RGBAParam*, kColorSwitchCount> switchNew_{};
};

void defineParams(OFX::ImageEffectDescriptor& desc, EffectKind kind) {
  OFX::PageParamDescriptor* page = desc.definePageParam("Controls");
  switch (kind) {
    case EffectKind::SelectiveColorBlur:
      defineInt(desc, *page, "blur", "Blur", 0, 0, 1000, 50);
      defineTargetColorGroup(desc, *page);
      break;
    case EffectKind::MainLineRepaint:
      defineColor(desc, *page, "mainColor", "Main Color", {0.0f, 0.0f, 0.0f, 1.0f});
      defineInt(desc, *page, "tolerance", "Tolerance", 0, 0, 255, 25);
      defineInt(desc, *page, "scanLength", "Scan Length", 16, 2, 512, 64);
      break;
    case EffectKind::SelectColor:
      defineTargetColorGroup(desc, *page);
      defineBool(desc, *page, "reverse", "Reverse", false);
      defineInt(desc, *page, "tolerance", "Tolerance", 0, 0, 255, 15);
      break;
    case EffectKind::ColorSwitch: {
      defineBool(desc, *page, "enableAll", "Enable All", true);
      defineInt(desc, *page, "activeCount", "Active Param Count", 8, 0, kColorSwitchCount, kColorSwitchCount);
      defineChoice(desc, *page, "mode", "Mode", 0, {"Replace", "Key", "Extract"});
      OFX::GroupParamDescriptor* group = desc.defineGroupParam("colors");
      group->setLabels("Colors", "Colors", "Colors");
      page->addChild(*group);
      for (int i = 0; i < kColorSwitchCount; ++i) {
        defineBool(desc, *page, colorSwitchEnabledName(i), "Switch " + std::to_string(i) + " On", false, group);
        defineColor(desc, *page, colorSwitchOldName(i), "Switch " + std::to_string(i) + " Old", {0.0f, 0.0f, 0.0f, 1.0f}, group);
        defineColor(desc, *page, colorSwitchNewName(i), "Switch " + std::to_string(i) + " New", {1.0f, 1.0f, 1.0f, 1.0f}, group);
      }
      break;
    }
    case EffectKind::EdgeLine:
      defineColor(desc, *page, "targetColor", "Target Color", {1.0f, 0.0f, 0.0f, 1.0f});
      defineColor(desc, *page, "sampleColor", "Sample Color", {0.0f, 1.0f, 0.0f, 1.0f});
      defineInt(desc, *page, "tolerance", "Tolerance", 0, 0, 255, 25);
      defineInt(desc, *page, "length", "Length", 10, 0, 200, 20);
      defineColor(desc, *page, "drawColor", "Draw Color", {0.0f, 0.0f, 1.0f, 1.0f});
      break;
    case EffectKind::RimFill:
      defineInt(desc, *page, "width", "Width", 1, 0, 200, 20);
      defineChoice(desc, *page, "mode", "Fill Method", 0, {"CustomColor", "AdjacentColor"});
      defineColor(desc, *page, "customColor", "Custom Color", {1.0f, 1.0f, 1.0f, 1.0f});
      defineBool(desc, *page, "whiteTransparent", "Treat White as Transparent", false);
      break;
    case EffectKind::LineExtraction:
      defineInt(desc, *page, "outerWidth", "Outer Sampling", 3, 1, 64, 16);
      defineInt(desc, *page, "innerWidth", "Inner Sampling", 1, 0, 63, 8);
      defineColor(desc, *page, "drawColor", "Color", {0.0f, 0.0f, 0.0f, 1.0f});
      defineBool(desc, *page, "postLevel", "Post Level", false);
      defineDouble(desc, *page, "levelLow", "Level Low", 0.0, 0.0, 1.0);
      defineDouble(desc, *page, "levelHigh", "Level High", 1.0, 0.0, 1.0);
      defineBool(desc, *page, "blend", "Blend with Original", false);
      break;
  }
}

template <EffectKind Kind>
struct SpecForKind;
template <>
struct SpecForKind<EffectKind::SelectiveColorBlur> { static constexpr EffectSpec spec = kSelectiveColorBlur; };
template <>
struct SpecForKind<EffectKind::MainLineRepaint> { static constexpr EffectSpec spec = kMainLineRepaint; };
template <>
struct SpecForKind<EffectKind::SelectColor> { static constexpr EffectSpec spec = kSelectColor; };
template <>
struct SpecForKind<EffectKind::ColorSwitch> { static constexpr EffectSpec spec = kColorSwitch; };
template <>
struct SpecForKind<EffectKind::EdgeLine> { static constexpr EffectSpec spec = kEdgeLine; };
template <>
struct SpecForKind<EffectKind::RimFill> { static constexpr EffectSpec spec = kRimFill; };
template <>
struct SpecForKind<EffectKind::LineExtraction> { static constexpr EffectSpec spec = kLineExtraction; };

template <EffectKind Kind>
class FsFactory final : public OFX::PluginFactoryHelper<FsFactory<Kind>> {
 public:
  FsFactory()
      : OFX::PluginFactoryHelper<FsFactory<Kind>>(SpecForKind<Kind>::spec.id,
                                                  SpecForKind<Kind>::spec.major,
                                                  SpecForKind<Kind>::spec.minor) {}

  void describe(OFX::ImageEffectDescriptor& desc) override {
    setCommonDescriptor(desc, SpecForKind<Kind>::spec);
  }

  void describeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum /*context*/) override {
    defineClips(desc);
    defineParams(desc, Kind);
  }

  OFX::ImageEffect* createInstance(OfxImageEffectHandle handle, OFX::ContextEnum /*context*/) override {
    return new FsPlugin(handle, Kind);
  }
};

}  // namespace

namespace OFX {
namespace Plugin {

void getPluginIDs(PluginFactoryArray& ids) {
  static FsFactory<EffectKind::SelectiveColorBlur> selectiveColorBlurFactory;
  static FsFactory<EffectKind::MainLineRepaint> mainLineRepaintFactory;
  static FsFactory<EffectKind::SelectColor> selectColorFactory;
  static FsFactory<EffectKind::ColorSwitch> colorSwitchFactory;
  static FsFactory<EffectKind::EdgeLine> edgeLineFactory;
  static FsFactory<EffectKind::RimFill> rimFillFactory;
  static FsFactory<EffectKind::LineExtraction> lineExtractionFactory;

  ids.push_back(&selectiveColorBlurFactory);
  ids.push_back(&mainLineRepaintFactory);
  ids.push_back(&selectColorFactory);
  ids.push_back(&colorSwitchFactory);
  ids.push_back(&edgeLineFactory);
  ids.push_back(&rimFillFactory);
  ids.push_back(&lineExtractionFactory);
}

}  // namespace Plugin
}  // namespace OFX
