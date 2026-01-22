#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Root Makefile for Rockbox builds
#
# Usage:
#   make              - Show help
#   make build        - Build Rockbox firmware for iPod Video (via Docker)
#   make rbutil       - Build Rockbox Utility for macOS (native)
#   make clean        - Remove build artifacts

.PHONY: help build rbutil clean

help:
	@echo "Rockbox Build System"
	@echo ""
	@echo "Usage:"
	@echo "  make build   - Build Rockbox firmware for iPod Video 5.5G (via Docker)"
	@echo "  make rbutil  - Build Rockbox Utility for macOS Apple Silicon (native)"
	@echo "  make clean   - Remove build artifacts from output/"
	@echo ""
	@echo "Artifacts are placed in output/"
	@echo ""
	@echo "For other targets or manual builds, see docs/README"

build:
	./tools/docker_ipodvideo/build.sh build

rbutil:
	./utils/rbutilqt/macos/build.sh build

clean:
	./tools/docker_ipodvideo/build.sh clean
	./utils/rbutilqt/macos/build.sh clean
