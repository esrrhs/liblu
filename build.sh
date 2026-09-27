#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BUILD_TYPE="${1:-Release}"

case "${BUILD_TYPE}" in
    Release|release)
        BUILD_TYPE="Release"
        ;;
    Debug|debug)
        BUILD_TYPE="Debug"
        ;;
    *)
        echo "usage: $0 [Release|Debug]"
        exit 1
        ;;
esac

echo "==> Configuring liblu (${BUILD_TYPE})..."
cmake -B "${BUILD_DIR}" -S "${SCRIPT_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

echo "==> Building liblu..."
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" -j"$(nproc 2>/dev/null || echo 2)"

echo "==> Running smoke test..."
ctest --test-dir "${BUILD_DIR}" --output-on-failure -C "${BUILD_TYPE}"

echo "==> Build successful"
echo "    library: ${BUILD_DIR}/bin/liblu.a"
echo "    test:    ${BUILD_DIR}/bin/lu_test"
