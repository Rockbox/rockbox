#!/bin/sh
unzip $1 rockbox.bin
dd if=bootloader-x3.bin of=rockbox.bin bs=2048 seek=4 conv=nocreat conv=notrunc
zip -o $1 rockbox.bin rockbox-info.txt
rm rockbox.bin
