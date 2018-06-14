#!/bin/sh
ROCKBOX_SRC=$HOME/rockbox
BUILD=$ROCKBOX_SRC/build
ISO=$HOME/iso
ROOTFS=$HOME/rootfs

[ -z "$UPT_DIR" ] && UPT_DIR=`pwd` 
cd $HOME

# get sources
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 0: Get sources !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
[ -d "$ROCKBOX_SRC" ] || git clone ttp://gerrit.rockbox.org/p/rockbox

cd $ROCKBOX_SRC

# build bootloader
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 1: Build bootloader !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo

[ -d "$BUILD" ] && rm -rf $BUILD
git pull && \
mkdir $BUILD && cd $BUILD && \
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
[ -d "$ISO" ] && rm -rf $ISO
mkdir $ISO && \
xorriso -osirrox on -ecma119_map lowercase -indev $UPT_DIR/update.upt -extract / $ISO

# Extract rootfs files. Preserve permissions (although this are wrong!)
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 3: Extract system.ubi !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
ubireader_extract_files -k -o $ROOTFS $ISO/system.ubi

# Copy rockbox bootloader
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 4: Copy bootloader !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
cp $BUILD/bootloader.rocker $ROOTFS/usr/bin/rb_bootloader

# Overwrite default player starting script with one running our bootloader
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 5: Modify startup script !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
cp $ROCKBOX_SRC/tools/agptek_rocker//hiby_player.sh $ROOTFS/usr/bin/hiby_player.sh && \
chmod 755 $ROOTFS/usr/bin/hiby_player.sh

# Rebuild ubifs
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 6: Rebuild system.ubi !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
mkfs.ubifs --min-io-size=2048 --leb-size=126976 --max-leb-cnt=1024 -o $HOME/system_rb.ubi -r $ROOTFS && \
mv $HOME/system_rb.ubi $ISO/system.ubi

# Fixup update.txt file with correct md5
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 7: Fixup update.txt !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
python $ROCKBOX_SRC/tools/agptek_rocker/update_update.py $ISO/update.txt

# Rebuild .upt file
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 8: Rebuild upt file !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
xorriso -as mkisofs -volid 'CDROM' --norock -output $UPT_DIR/update_rb.upt $ISO

# Build rockbox.zip
echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! STEP 9: Build rockbox application !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo
cd $BUILD && \
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
rm -rf $BUILD
rm -rf $ISO
rm -rf $ROOTFS

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
