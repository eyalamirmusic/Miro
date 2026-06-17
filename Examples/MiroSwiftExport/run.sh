#!/usr/bin/env bash
#
# End-to-end Swift -> C++ integration check, built entirely by CMake.
#
# CMake's Swift support needs the Ninja generator (the default Unix
# Makefiles generator has none), so this uses a dedicated build dir,
# separate from the repo's default `build/`.
#
# The CalcDemo target: generates Schema.swift + Schema.client.swift from
# CalcApi (miro_export), builds the C++ host shared library, compiles the
# Swift app against the generated sources + the bridging header, links the
# host, and registers a CTest. Every call in main.swift crosses
# Swift -> C ABI -> Miro::Bridge -> C++ and back.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build-swift"

if ! command -v swiftc >/dev/null 2>&1; then
    echo "swiftc not found — skipping the Swift integration check."
    exit 0
fi

if ! command -v ninja >/dev/null 2>&1; then
    echo "ninja not found — CMake Swift support needs it. Install ninja and retry."
    exit 1
fi

echo "==> Configuring (Ninja, examples on)"
cmake -B "$BUILD_DIR" -S "$ROOT_DIR" -G Ninja \
    -DMIRO_UNITY_BUILD=OFF -DMIRO_BUILD_EXAMPLES=ON

echo "==> Building CalcDemo"
cmake --build "$BUILD_DIR" --target CalcDemo

echo "==> Running the integration check"
echo "-------------------------------------"
ctest --test-dir "$BUILD_DIR" -R CalcDemo --output-on-failure
