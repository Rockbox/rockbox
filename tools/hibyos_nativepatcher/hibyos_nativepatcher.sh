#!/bin/bash
# USAGE: ./hibyos_nativepatcher.sh <path/to/updatefile.upt> <path/to/bootloader.erosq>
# NOTE: THIS SCRIPT IS NOT TOLERANT OF WHITESPACE IN FILENAMES OR PATHS

if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "$OSTYPE DETECTED"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "$OSTYPE DETECTED"
    if !(which 7z > /dev/null); then
        echo "PLEASE INSTALL 7z (usually part of p7zip-full package)"
        exit 1
    fi
    if !(which genisoimage > /dev/null); then
        echo "PLEASE INSTALL genisoimage"
        exit 1
    fi
else
    echo "SCRIPT NOT IMPLEMENTED ON $OSTYPE YET!"
    exit 1
fi

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
if !(which $SCRIPT_DIR/patch_manifest.pl > /dev/null); then
    echo "couldn't find patch_manifest.pl!"
    exit 1
fi

updatefile=$(realpath --relative-base=$(pwd) $1)
updatefile_path=$(echo "$updatefile" | perl -ne "s/\/[\w\.\_\-]*$// && print")
updatefile_name=$(basename $updatefile)
updatefile_name_noext=$(echo "$updatefile_name" | perl -ne "s/\.\w*$// && print")
bootfile=$(realpath --relative-base=$(pwd) $2)
echo "This will patch $updatefile with $bootfile..."

echo "MAKE WORKING DIR..."
mkdir $updatefile_path/working_dir
working_dir=$(realpath $updatefile_path/working_dir)

# copy update.upt to update.iso
cp $updatefile $working_dir/$updatefile_name_noext\_cpy.iso

mkdir $working_dir/image_contents

# attach/mount iso
echo "mount/extract/unmount original iso..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macos
    hdiutil attach $working_dir/$updatefile_name_noext\_cpy.iso -mountpoint $working_dir/contentsiso

    # copy out iso contents
    cp $working_dir/contentsiso/* $working_dir/image_contents

    # unmount iso
    hdiutil detach $working_dir/contentsiso
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # linux-gnu
    7z -o$working_dir/image_contents x $working_dir/$updatefile_name_noext\_cpy.iso
fi

chmod 777 $working_dir/image_contents/*

# extract spl, bootloader
echo "extract bootloader..."
mkdir $working_dir/bootloader

if [[ "$OSTYPE" == "darwin"* ]]; then
    # macos
    tar -xvf $bootfile --cd $working_dir/bootloader
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # linux-gnu
    tar -xvf $bootfile -C $working_dir/bootloader
fi

bootver=$(cat $working_dir/bootloader/bootloader-info.txt)
if [ -z "$bootver" ]; then
    echo "COULDN'T FIND BOOTLOADER-INFO!"
    echo "cleaning up..."
    rm -rf $working_dir
    exit 1
fi
echo "FOUND VERSION $bootver"

# patch uboot.bin
echo "PATCH!"
dd if=$working_dir/bootloader/spl.erosq of=$working_dir/image_contents/uboot.bin obs=1 seek=0 conv=notrunc
dd if=$working_dir/bootloader/bootloader.ucl of=$working_dir/image_contents/uboot.bin obs=1 seek=26624 conv=notrunc

# modify update.txt
md5=($(md5sum $working_dir/image_contents/uboot.bin))
if [ -z "$md5" ]; then
    echo "COULDN'T MD5SUM UBOOT.BIN!"
    echo "cleaning up..."
    rm -rf $working_dir
    exit 1
fi
echo "add to update manifest with md5sum $md5"
$SCRIPT_DIR/patch_manifest.pl $md5 $working_dir/image_contents/update.txt

# modify version.txt?

# create iso
echo "make new iso..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macos
    hdiutil makehybrid -iso -joliet -o $working_dir/$updatefile_name_noext\_patched_$bootver.iso $working_dir/image_contents/
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # linux-gnu
    genisoimage -o $working_dir/$updatefile_name_noext\_patched_$bootver.iso $working_dir/image_contents/
fi

# rename to something.upt and put in original directory
echo "final output file $updatefile_name_noext\_patched_$bootver.upt"
mv $working_dir/$updatefile_name_noext\_patched_$bootver.iso $updatefile_path/$updatefile_name_noext\_patched_$bootver.upt

# clean up
echo "cleaning up..."
rm -rf $working_dir
