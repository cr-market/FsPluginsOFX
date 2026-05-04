cmake_minimum_required(VERSION 3.20)

project(FsPluginsOFX VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_OFX_PLUGIN "Build the OpenFX plugin bundle" ON)
set(OPENFX_ROOT "${CMAKE_CURRENT_LIST_DIR}/../openfx-sdk" CACHE PATH "Path to the OpenFX SDK checkout")

add_library(selective_color_blur_core STATIC
  src/SelectiveColorBlurCore.cpp
  src/SelectiveColorBlurCore.h)
target_include_directories(selective_color_blur_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_executable(selective_color_blur_core_test tests/core_test.cpp)
target_link_libraries(selective_color_blur_core_test PRIVATE selective_color_blur_core)
enable_testing()
add_test(NAME selective_color_blur_core_test COMMAND selective_color_blur_core_test)

if(BUILD_OFX_PLUGIN)
  if(NOT EXISTS "${OPENFX_ROOT}/include/ofxImageEffect.h")
    message(FATAL_ERROR "OPENFX_ROOT must point to an OpenFX SDK checkout. Current value: ${OPENFX_ROOT}")
  endif()

  set(OFX_HEADER_DIR "${OPENFX_ROOT}/include")
  set(OFX_SUPPORT_HEADER_DIR "${OPENFX_ROOT}/Support/include")
  set(OFX_SUPPORT_LIBRARY_DIR "${OPENFX_ROOT}/Support/Library")

  file(GLOB OFX_SUPPORT_LIBRARY_FILES "${OFX_SUPPORT_LIBRARY_DIR}/*.cpp")
  add_library(OfxSupport STATIC ${OFX_SUPPORT_LIBRARY_FILES})
  target_compile_features(OfxSupport PUBLIC cxx_std_17)
  target_include_directories(OfxSupport PUBLIC "${OFX_HEADER_DIR}" "${OFX_SUPPORT_HEADER_DIR}")
  if(NOT MSVC)
    target_compile_options(OfxSupport PRIVATE -fPIC)
  endif()

  if(APPLE)
    set(OFX_ARCH_DIR "MacOS")
    add_library(FsPluginsOFX MODULE src/SelectiveColorBlurPlugin.cpp)
    set_target_properties(FsPluginsOFX PROPERTIES BUNDLE OFF)
    target_link_options(FsPluginsOFX PRIVATE -undefined dynamic_lookup)
  elseif(WIN32)
    set(OFX_ARCH_DIR "Win64")
    add_library(FsPluginsOFX SHARED src/SelectiveColorBlurPlugin.cpp)
  else()
    set(OFX_ARCH_DIR "Linux-x86-64")
    add_library(FsPluginsOFX SHARED src/SelectiveColorBlurPlugin.cpp)
  endif()

  target_link_libraries(FsPluginsOFX PRIVATE selective_color_blur_core OfxSupport)
  target_include_directories(FsPluginsOFX PRIVATE "${OFX_HEADER_DIR}" "${OFX_SUPPORT_HEADER_DIR}")
  target_compile_definitions(FsPluginsOFX PRIVATE OFX_EXTENSIONS_NUKE)
  set_target_properties(FsPluginsOFX PROPERTIES PREFIX "" SUFFIX ".ofx")

  set(OFX_BUNDLE_DIR "${CMAKE_BINARY_DIR}/FsPluginsOFX.ofx.bundle")
  add_custom_command(TARGET FsPluginsOFX POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${OFX_BUNDLE_DIR}/Contents/${OFX_ARCH_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:FsPluginsOFX>" "${OFX_BUNDLE_DIR}/Contents/${OFX_ARCH_DIR}/FsPluginsOFX.ofx"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist" "${OFX_BUNDLE_DIR}/Contents/Info.plist"
    COMMENT "Assembling FsPluginsOFX.ofx.bundle")
endif()
