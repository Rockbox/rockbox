#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Root Makefile for Docker-based builds
#
# Usage:
#   make              - Show help
#   make build        - Build Rockbox for iPod Video (via Docker)
#   make clean        - Remove build artifacts

.PHONY: help build clean

help:
	@echo "Rockbox Docker Build"
	@echo ""
	@echo "Usage:"
	@echo "  make build   - Build Rockbox for iPod Video 5.5G (via Docker)"
	@echo "  make clean   - Remove build artifacts from output/"
	@echo ""
	@echo "Artifacts are placed in output/"
	@echo ""
	@echo "For other targets or manual builds, see docs/README"

build:
	./tools/docker_ipodvideo/build.sh build

clean:
	./tools/docker_ipodvideo/build.sh clean
