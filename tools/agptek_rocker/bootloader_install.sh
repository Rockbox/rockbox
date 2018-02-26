#!/bin/sh

[ -z "$UPT_DIR" ] && UPT_DIR=`pwd` 
cd $HOME

# get sources
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 0: Get sources !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
[ -d "$HOME/rockbox-wodz" ] || git clone https://github.com/wodz/rockbox-wodz.git

cd $HOME/rockbox-wodz

# build bootloader
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 1: Build bootloader !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo

[ -d "$HOME/rockbox-wodz/build" ] && rm -rf $HOME/rockbox-wodz/build
git checkout agptek-rocker && \
mkdir $HOME/rockbox-wodz/build && cd $HOME/rockbox-wodz/build && \
../tools/configure --target=240 --type=b && \
make clean && \
make && \
cd $HOME

# Extract update file (ISO9660 image) content
# NOTE: Update process on device loop mount ISO image. Default behavior of mount
#       is to map all names to lowercase. Because of this forcing lowercase
#       mapping is needed when extracting
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 2: Extract upt file !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
[ -d "$HOME/iso" ] && rm -rf $HOME/iso
mkdir $HOME/iso && \
xorriso -osirrox on -ecma119_map lowercase -indev $UPT_DIR/update.upt -extract / $HOME/iso

# Extract rootfs files. Preserve permissions (although this are wrong!)
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 3: Extract system.ubi !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
ubireader_extract_files -k -o $HOME/rootfs $HOME/iso/system.ubi

# Copy rockbox bootloader
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 4: Copy bootloader !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
cp $HOME/rockbox-wodz/build/bootloader.elf $HOME/rootfs/usr/bin/rb_bootloader && \
mipsel-rockbox-linux-gnu-strip --strip-unneeded $HOME/rootfs/usr/bin/rb_bootloader

# Overwrite default player starting script with one running our bootloader
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 5: Modify startup script !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
cp $HOME/rockbox-wodz/tools/agptek_rocker//hiby_player.sh $HOME/rootfs/usr/bin/hiby_player.sh && \
chmod 755 $HOME/rootfs/usr/bin/hiby_player.sh

# Rebuild ubifs
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 6: Rebuild system.ubi !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
mkfs.ubifs --min-io-size=2048 --leb-size=126976 --max-leb-cnt=1024 -o $HOME/system_rb.ubi -r $HOME/rootfs && \
mv $HOME/system_rb.ubi $HOME/iso/system.ubi

# Fixup update.txt file with correct md5
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 7: Fixup update.txt !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
python $HOME/rockbox-wodz/tools/agptek_rocker/update_update.py $HOME/iso/update.txt

# Rebuild .upt file
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 8: Rebuild upt file !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
xorriso -as mkisofs -volid 'CDROM' --norock -output $UPT_DIR/update_rb.upt $HOME/iso

# Build rockbox.zip
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 9: Build rockbox application !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
cd $HOME/rockbox-wodz/build && \
../tools/configure --target=240 --type=n && \
make clean && \
make && \
make zip && \
cp rockbox.zip $UPT_DIR/

# Cleanup
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 10: Cleanup !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!"
echo
rm -rf $HOME/rockbox-wodz/build
rm -rf $HOME/iso
rm -rf $HOME/rootfs

echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!                  Building finished                               !"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
echo "You should find update_rb.upt and rockbox.zip in output directory"
echo
echo "1) Unzip rockbox.zip file in the root directory of SD card"
echo "2) Copy update_rb.upt to the root directory of SD card"
echo "3) Rename update_rb.upt to update.upt in SD card"
echo "4) Select update firmware on device"
echo
