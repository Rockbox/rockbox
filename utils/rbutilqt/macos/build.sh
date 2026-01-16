#!/bin/bash
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Build script for Rockbox Utility on macOS (Apple Silicon)
#
# Usage:
#   ./build.sh [build|deploy|clean]
#
#   build   - Build RockboxUtility.app (default)
#   deploy  - Build and create DMG for distribution
#   clean   - Remove build directory
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROCKBOX_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
BUILD_DIR="$ROCKBOX_ROOT/build-rbutil"
OUTPUT_DIR="$ROCKBOX_ROOT/output"

usage() {
    echo "Usage: $0 [build|deploy|clean]"
    echo ""
    echo "  build   - Build RockboxUtility.app (default)"
    echo "  deploy  - Build and create DMG for distribution"
    echo "  clean   - Remove build directory"
    exit 1
}

check_deps() {
    local missing=""

    if ! command -v brew &> /dev/null; then
        echo "Error: Homebrew is required. Install from https://brew.sh"
        exit 1
    fi

    if ! command -v cmake &> /dev/null; then
        missing="$missing cmake"
    fi

    # Check for Qt6
    if ! brew list qt@6 &> /dev/null; then
        missing="$missing qt@6"
    fi

    if [ -n "$missing" ]; then
        echo "Missing dependencies:$missing"
        echo ""
        echo "Install with: brew install$missing"
        exit 1
    fi
}

do_build() {
    check_deps

    echo "Building Rockbox Utility for macOS..."

    # Get Qt path from Homebrew
    QT_DIR="$(brew --prefix qt@6)"

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    cmake "$ROCKBOX_ROOT/utils" \
        -DCMAKE_PREFIX_PATH="$QT_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_OSX_ARCHITECTURES=arm64

    make -j$(sysctl -n hw.ncpu) RockboxUtility

    echo ""
    echo "Build complete!"
    echo "App bundle: $BUILD_DIR/rbutilqt/RockboxUtility.app"
}

do_deploy() {
    check_deps

    # Also need python3 for dmgbuild
    if ! command -v python3 &> /dev/null; then
        echo "Error: python3 is required for deployment"
        exit 1
    fi

    echo "Building Rockbox Utility DMG for macOS..."

    # Get Qt path from Homebrew
    QT_DIR="$(brew --prefix qt@6)"

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    cmake "$ROCKBOX_ROOT/utils" \
        -DCMAKE_PREFIX_PATH="$QT_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_OSX_ARCHITECTURES=arm64

    make -j$(sysctl -n hw.ncpu) deploy_RockboxUtility

    # Copy DMG to output
    mkdir -p "$OUTPUT_DIR"
    cp -v "$BUILD_DIR/RockboxUtility.dmg" "$OUTPUT_DIR/"

    echo ""
    echo "Deploy complete!"
    echo "DMG: $OUTPUT_DIR/RockboxUtility.dmg"
}

do_clean() {
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    rm -f "$OUTPUT_DIR/RockboxUtility.dmg"
    rm -f "$OUTPUT_DIR/RockboxUtility.app"
    echo "Done."
}

case "${1:-build}" in
    build)
        do_build
        ;;
    deploy)
        do_deploy
        ;;
    clean)
        do_clean
        ;;
    -h|--help|help)
        usage
        ;;
    *)
        echo "Error: Unknown command '$1'"
        usage
        ;;
esac
