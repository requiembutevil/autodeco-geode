#!/bin/bash
set -euo pipefail

if ! command -v cmake >/dev/null 2>&1; then
  echo "CMake was not found on PATH. Run: cmake --version"
  exit 1
fi
if ! command -v ninja >/dev/null 2>&1; then
  echo "Ninja is required for this build. Install it with: brew install ninja"
  exit 1
fi
if ! command -v xcrun >/dev/null 2>&1 || ! xcrun --sdk iphoneos --show-sdk-path >/dev/null 2>&1; then
  echo "The iPhoneOS SDK was not found. Install/open Xcode, then run: sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer"
  exit 1
fi
if [[ -z "${GEODE_SDK:-}" || ! -d "${GEODE_SDK:-}" ]]; then
  echo "GEODE_SDK is not set to an SDK folder. Run 'geode sdk install', restart Terminal, and try again."
  exit 1
fi

cmake -B build-ios -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  '-DGEODE_CODEGEN_CMAKE_ARGS=-DCMAKE_C_COMPILER=clang;-DCMAKE_CXX_COMPILER=clang++;-G Ninja' \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DGEODE_TARGET_PLATFORM=iOS \
  -DGEODE_DISABLE_PRECOMPILED_HEADERS=OFF \
  -DGEODE_DONT_INSTALL_MODS=ON

cmake --build build-ios --config Release --parallel

echo
echo "Build finished. Locate the .geode file with:"
find build-ios -name '*.geode' -print
