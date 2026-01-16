#!/bin/bash
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Install Rockbox onto a connected iPod (macOS)
#
# Usage:
#   ./install-ipod.sh [--bootloader-only | --files-only]
#
# This script:
#   1. Detects the connected iPod
#   2. Installs the Rockbox bootloader (if not already installed)
#   3. Copies the Rockbox firmware files to the iPod
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROCKBOX_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
OUTPUT_DIR="$ROCKBOX_ROOT/output"
BUILD_RBUTIL="$ROCKBOX_ROOT/build-rbutil"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

info() { echo -e "${BLUE}==>${NC} $1"; }
success() { echo -e "${GREEN}==>${NC} $1"; }
warn() { echo -e "${YELLOW}Warning:${NC} $1"; }
error() { echo -e "${RED}Error:${NC} $1"; exit 1; }

# Check if running as root (needed for bootloader installation)
check_root() {
    if [ "$EUID" -ne 0 ]; then
        error "This script must be run as root (sudo) to install the bootloader.
       Run: sudo $0 $*"
    fi
}

# Find ipodpatcher binary
find_ipodpatcher() {
    # Check if built
    if [ -x "$BUILD_RBUTIL/ipodpatcher" ]; then
        echo "$BUILD_RBUTIL/ipodpatcher"
        return 0
    fi

    # Check common locations
    for path in \
        "$BUILD_RBUTIL/utils/ipodpatcher" \
        "/usr/local/bin/ipodpatcher" \
        "/opt/homebrew/bin/ipodpatcher"; do
        if [ -x "$path" ]; then
            echo "$path"
            return 0
        fi
    done

    return 1
}

# Build ipodpatcher if needed
ensure_ipodpatcher() {
    if IPODPATCHER=$(find_ipodpatcher); then
        return 0
    fi

    info "Building ipodpatcher..."
    mkdir -p "$BUILD_RBUTIL"
    cd "$BUILD_RBUTIL"

    QT_DIR="$(brew --prefix qt@6 2>/dev/null || echo "")"
    if [ -n "$QT_DIR" ]; then
        cmake "$ROCKBOX_ROOT/utils" -DCMAKE_PREFIX_PATH="$QT_DIR" -DCMAKE_BUILD_TYPE=Release
    else
        cmake "$ROCKBOX_ROOT/utils" -DCMAKE_BUILD_TYPE=Release
    fi

    make -j$(sysctl -n hw.ncpu) ipodpatcher-bin

    IPODPATCHER="$BUILD_RBUTIL/ipodpatcher"
    if [ ! -x "$IPODPATCHER" ]; then
        error "Failed to build ipodpatcher"
    fi
}

# Detect connected iPod
detect_ipod() {
    info "Scanning for connected iPods..."

    # Look for iPod volumes
    IPOD_MOUNT=""
    IPOD_DISK=""

    for vol in /Volumes/*; do
        if [ -d "$vol" ]; then
            # Check if it looks like an iPod (has iPod_Control or .rockbox)
            if [ -d "$vol/iPod_Control" ] || [ -d "$vol/.rockbox" ]; then
                IPOD_MOUNT="$vol"
                # Get the disk device from the mount
                IPOD_DISK=$(diskutil info "$vol" 2>/dev/null | grep "Device Node" | awk '{print $3}' | sed 's/s[0-9]*$//')
                break
            fi
        fi
    done

    if [ -z "$IPOD_MOUNT" ]; then
        # Try scanning with ipodpatcher
        if [ -x "$IPODPATCHER" ]; then
            info "No mounted iPod found, scanning disk devices..."
            "$IPODPATCHER" --scan 2>&1 || true
        fi
        error "No iPod found. Please ensure your iPod is:
       1. Connected via USB
       2. In disk mode (if needed: hold Menu+Select to reboot, then Select+Play for disk mode)
       3. Mounted as a drive"
    fi

    success "Found iPod at $IPOD_MOUNT (disk: $IPOD_DISK)"
}

# Check if bootloader is already installed
check_bootloader() {
    info "Checking bootloader status..."

    # Use ipodpatcher to list firmware - look for rockbox signature
    if "$IPODPATCHER" "$IPOD_DISK" -l 2>&1 | grep -qi "rockbox"; then
        return 0  # Bootloader installed
    fi
    return 1  # No bootloader
}

# Install bootloader
install_bootloader() {
    info "Installing Rockbox bootloader..."

    # Check for bootloader file
    BOOTLOADER="$OUTPUT_DIR/bootloader-ipodvideo.ipod"

    if [ -f "$BOOTLOADER" ]; then
        info "Using bootloader from $BOOTLOADER"
        "$IPODPATCHER" "$IPOD_DISK" -a "$BOOTLOADER"
    else
        # Use embedded bootloader (if ipodpatcher was built with BOOTOBJS)
        info "Using embedded bootloader..."
        "$IPODPATCHER" "$IPOD_DISK" --install
    fi

    success "Bootloader installed successfully!"
}

# Install firmware files
install_files() {
    ROCKBOX_ZIP="$OUTPUT_DIR/rockbox.zip"

    if [ ! -f "$ROCKBOX_ZIP" ]; then
        error "rockbox.zip not found at $ROCKBOX_ZIP
       Run 'make build' first to build the firmware."
    fi

    info "Installing Rockbox files to $IPOD_MOUNT..."

    # Check available space
    REQUIRED_MB=50  # Approximate size needed
    AVAILABLE_MB=$(df -m "$IPOD_MOUNT" | tail -1 | awk '{print $4}')
    if [ "$AVAILABLE_MB" -lt "$REQUIRED_MB" ]; then
        error "Not enough space on iPod. Need ${REQUIRED_MB}MB, have ${AVAILABLE_MB}MB"
    fi

    # Remove old installation if present
    if [ -d "$IPOD_MOUNT/.rockbox" ]; then
        info "Removing previous Rockbox installation..."
        rm -rf "$IPOD_MOUNT/.rockbox"
    fi

    # Extract with progress
    info "Extracting rockbox.zip..."
    unzip -o "$ROCKBOX_ZIP" -d "$IPOD_MOUNT/" | while read -r line; do
        # Show progress for major directories
        if echo "$line" | grep -q "creating:"; then
            dir=$(echo "$line" | sed 's/.*creating: //' | cut -d'/' -f1-2)
            echo -ne "\r  Extracting: $dir                    "
        fi
    done
    echo ""  # New line after progress

    # Verify installation
    if [ -f "$IPOD_MOUNT/.rockbox/rockbox.ipod" ]; then
        success "Firmware files installed successfully!"
    else
        error "Installation verification failed - rockbox.ipod not found"
    fi
}

# Sync and eject
finish_install() {
    info "Syncing filesystem..."
    sync

    info "Ejecting iPod..."
    diskutil eject "$IPOD_DISK" 2>/dev/null || diskutil unmount "$IPOD_MOUNT" 2>/dev/null || true

    echo ""
    success "============================================"
    success "  Rockbox installation complete!"
    success "============================================"
    echo ""
    echo "Your iPod is ready. It will boot into Rockbox on next startup."
    echo ""
    echo "To boot original Apple firmware: hold MENU while booting"
    echo "To enter disk mode: hold SELECT+PLAY while booting"
    echo ""
}

# Main installation flow
main() {
    local INSTALL_BOOTLOADER=true
    local INSTALL_FILES=true

    # Parse arguments
    case "${1:-}" in
        --bootloader-only)
            INSTALL_FILES=false
            ;;
        --files-only)
            INSTALL_BOOTLOADER=false
            ;;
        --help|-h)
            echo "Usage: $0 [--bootloader-only | --files-only]"
            echo ""
            echo "Install Rockbox onto a connected iPod."
            echo ""
            echo "Options:"
            echo "  --bootloader-only   Only install the bootloader"
            echo "  --files-only        Only copy firmware files (skip bootloader)"
            echo ""
            exit 0
            ;;
    esac

    echo ""
    info "Rockbox iPod Installer"
    echo ""

    # Need root for bootloader installation
    if [ "$INSTALL_BOOTLOADER" = true ]; then
        check_root
    fi

    # Ensure ipodpatcher is available
    ensure_ipodpatcher

    # Detect iPod
    detect_ipod

    # Install bootloader if needed
    if [ "$INSTALL_BOOTLOADER" = true ]; then
        if check_bootloader; then
            success "Bootloader already installed, skipping..."
        else
            install_bootloader
        fi
    fi

    # Install firmware files
    if [ "$INSTALL_FILES" = true ]; then
        install_files
    fi

    # Finish up
    finish_install
}

main "$@"
