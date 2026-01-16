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
#   make install      - Install Rockbox onto connected iPod (requires sudo)
#   make rbutil       - Build Rockbox Utility for macOS (native)
#   make clean        - Remove build artifacts

.PHONY: help build install rbutil clean

help:
	@echo "Rockbox Build System"
	@echo ""
	@echo "Usage:"
	@echo "  make build   - Build Rockbox firmware for iPod Video 5.5G (via Docker)"
	@echo "  make install - Install Rockbox onto connected iPod (requires sudo)"
	@echo "  make rbutil  - Build Rockbox Utility for macOS Apple Silicon (native)"
	@echo "  make clean   - Remove build artifacts from output/"
	@echo ""
	@echo "Typical workflow:"
	@echo "  1. make build    # Build firmware"
	@echo "  2. make install  # Install to iPod (run with sudo)"
	@echo ""
	@echo "Artifacts are placed in output/"

build:
	./tools/docker_ipodvideo/build.sh build

install:
	@echo "Note: Installing bootloader requires root access."
	@echo "Run: sudo make install"
	@echo ""
	sudo ./tools/install-ipod.sh

rbutil:
	./utils/rbutilqt/macos/build.sh build

clean:
	./tools/docker_ipodvideo/build.sh clean
	./utils/rbutilqt/macos/build.sh clean
