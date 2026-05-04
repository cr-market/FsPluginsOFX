OPENFX_ROOT ?= ../openfx-sdk
BUILD_DIR ?= build
CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -fPIC -Wall -Wextra
CPPFLAGS += -Isrc -I$(OPENFX_ROOT)/include -I$(OPENFX_ROOT)/Support/include

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  OFX_ARCH_DIR := MacOS
  LDFLAGS_PLUGIN := -bundle -undefined dynamic_lookup
else ifeq ($(OS),Windows_NT)
  OFX_ARCH_DIR := Win64
  LDFLAGS_PLUGIN := -shared
else
  OFX_ARCH_DIR := Linux-x86-64
  LDFLAGS_PLUGIN := -shared
endif

CORE_OBJ := $(BUILD_DIR)/SelectiveColorBlurCore.o
PLUGIN_OBJ := $(BUILD_DIR)/SelectiveColorBlurPlugin.o
SUPPORT_SRCS := $(wildcard $(OPENFX_ROOT)/Support/Library/*.cpp)
SUPPORT_OBJS := $(patsubst $(OPENFX_ROOT)/Support/Library/%.cpp,$(BUILD_DIR)/ofxs_%.o,$(SUPPORT_SRCS))
TEST_OBJ := $(BUILD_DIR)/core_test.o
BUNDLE := $(BUILD_DIR)/FsPluginsOFX.ofx.bundle
PLUGIN_BIN := $(BUNDLE)/Contents/$(OFX_ARCH_DIR)/FsPluginsOFX.ofx

.PHONY: all test clean bundle

all: test bundle

bundle: $(PLUGIN_BIN)

test: $(BUILD_DIR)/selective_color_blur_core_test
	$<

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(CORE_OBJ): src/SelectiveColorBlurCore.h src/ThinPattern.inc
$(PLUGIN_OBJ): src/SelectiveColorBlurCore.h

$(BUILD_DIR)/ofxs_%.o: $(OPENFX_ROOT)/Support/Library/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/core_test.o: tests/core_test.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/selective_color_blur_core_test: $(CORE_OBJ) $(TEST_OBJ)
	$(CXX) $^ -o $@

$(PLUGIN_BIN): $(CORE_OBJ) $(PLUGIN_OBJ) $(SUPPORT_OBJS) Info.plist
	mkdir -p "$(dir $@)" "$(BUNDLE)/Contents" "$(BUNDLE)/Contents/Resources"
	$(CXX) $(LDFLAGS_PLUGIN) $(CORE_OBJ) $(PLUGIN_OBJ) $(SUPPORT_OBJS) -o $@
	cp Info.plist "$(BUNDLE)/Contents/Info.plist"

clean:
	rm -rf "$(BUILD_DIR)"
