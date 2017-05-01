#!/bin/bash -e

# Compile and install the Rockbox toolchain
# s   - sh       (Archos models)
# m   - m68k     (iriver h1x0/h3x0, iaudio m3/m5/x5 and mpio hd200)
# a   - arm      (ipods, iriver H10, Sansa, D2, Gigabeat, etc)
# i   - mips     (Jz4740 and ATJ-based players)
# r   - arm-app  (Samsung ypr0)

# MIPS toolchain fails at installation: https://pastebin.com/raw/8SBhN58q
# ARM-APP toolchain is not tested
echo s m a | RBDEV_PREFIX="${HOME}/rbdev-toolchain" /rockbox/tools/rockboxdev.sh
echo 'PATH="$HOME/rbdev-toolchain/bin:$PATH"' >> "${HOME}/.profile"

# Download SDL-1.2.5 and compile it using MinGW32
cd "${HOME}"
wget --progress=bar:force http://www.libsdl.org/release/SDL-1.2.15.tar.gz
tar -zxvf SDL-1.2.15.tar.gz
cd SDL-1.2.15
./configure --host=x86_64-w64-mingw32 --prefix="${HOME}/mingw32-sdl"
make install
