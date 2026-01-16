#!/bin/bash
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Docker build script for iPod Video (5.5th gen)
#
# Usage:
#   ./build.sh [build|clean]
#
#   build  - Build rockbox.ipod and rockbox.zip (default)
#   clean  - Remove output directory
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROCKBOX_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
IMAGE_NAME="rockbox-ipodvideo"
OUTPUT_DIR="$ROCKBOX_ROOT/output"

usage() {
    echo "Usage: $0 [build|clean]"
    echo ""
    echo "  build  - Build rockbox.ipod and rockbox.zip (default)"
    echo "  clean  - Remove output directory"
    exit 1
}

build_image() {
    echo "Building Docker image '$IMAGE_NAME'..."
    docker build -t "$IMAGE_NAME" "$SCRIPT_DIR"
}

ensure_image() {
    if ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
        build_image
    fi
}

do_build() {
    ensure_image
    mkdir -p "$OUTPUT_DIR"

    echo "Building Rockbox for iPod Video..."
    docker run --rm \
        -v "$ROCKBOX_ROOT:/src:ro" \
        -v "$OUTPUT_DIR:/output" \
        "$IMAGE_NAME" \
        /bin/bash -c '
            set -e
            # Copy source to writable location (build modifies tools/)
            cp -a /src /rockbox
            mkdir -p /rockbox/build
            cd /rockbox/build
            ../tools/configure --target=22 --type=N
            make -j$(nproc)
            make zip
            cp -v rockbox.ipod /output/
            cp -v rockbox.zip /output/
        '

    echo ""
    echo "Build complete! Artifacts in: $OUTPUT_DIR"
    ls -la "$OUTPUT_DIR"
}

do_clean() {
    echo "Cleaning output directory..."
    rm -rf "$OUTPUT_DIR"
    echo "Done."
}

case "${1:-build}" in
    build)
        do_build
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
