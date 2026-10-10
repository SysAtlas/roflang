#!/usr/bin/env bash
set -e

BUILD_TESTING=OFF

for arg in "$@"; do
    case "$arg" in
        --test)
            BUILD_TESTING=ON
            ;;
        *)
            echo "Unknown argument: $arg" >&2
            exit 1
            ;;
    esac
done

cmake -S . -B ./build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DBUILD_TESTING="$BUILD_TESTING"

cmake --build ./build -j

if [[ "$BUILD_TESTING" == "ON" ]]; then
    ctest --test-dir ./build --output-on-failure
fi