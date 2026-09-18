#!/bin/sh
# Build the iPod Nano 3G NAND check image from this Rockbox tree.
#
#   build.sh check [OUTDIR]
#
# The check identifies the chip, tries a read-only mount, then serves the
# raw NAND over USB for nandcheck.py.
#
# Needs an ARM toolchain (Rockbox's from tools/rockboxdev.sh, or
# arm-none-eabi-gcc) and, for mks5lboot, libusb-1.0 development files.
# Writes nano3g-WHAT.dfu, to send with
#   utils/mks5lboot/mks5lboot --dfusend nano3g-WHAT.dfu
# from Apple's DFU mode, and nano3g-WHAT-wind3x.dfu if wInd3x is on PATH.
# Nothing is installed on the iPod and nothing is written to its NAND.
set -e

what=${1:-check}
case "$what" in
    check) define=NAND_CHECK ;;
    *)     echo "usage: $0 check [OUTDIR]" >&2; exit 2 ;;
esac

root=$(cd "$(dirname "$0")/../../.." && pwd)
mkdir -p "${2:-.}"
out=$(cd "${2:-.}" && pwd)
build="$root/build-nano3g-$what"
jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)

# Rockbox's own toolchain if it is on PATH, else a distribution's
# arm-none-eabi one; CROSS=prefix- overrides both
if [ -z "$CROSS" ] && ! command -v arm-elf-eabi-gcc > /dev/null 2>&1 \
   && command -v arm-none-eabi-gcc > /dev/null 2>&1; then
    CROSS=arm-none-eabi-
fi

mkdir -p "$build"
cd "$build"
if [ ! -f Makefile ]; then
    "$root/tools/configure" --target=ipodnano3g --type=b \
        ${CROSS:+--compiler-prefix=$CROSS} < /dev/null > configure.log 2>&1
fi
# The image is the Nano 3G bootloader built with one extra define
if ! grep -q -- "-D$define" Makefile; then
    sed "s|^export EXTRA_DEFINES=\(.*\)|export EXTRA_DEFINES=\1 -D$define|" \
        Makefile > Makefile.new && mv Makefile.new Makefile
fi
make -j"$jobs"

make -C "$root/utils/mks5lboot" > /dev/null
"$root/utils/mks5lboot/mks5lboot" --mkdfu-raw bootloader.bin \
    "$out/nano3g-$what.dfu"
if command -v wInd3x > /dev/null 2>&1; then
    wInd3x makedfu -k n3g bootloader.bin "$out/nano3g-$what-wind3x.dfu"
fi
echo "built $out/nano3g-$what.dfu"
