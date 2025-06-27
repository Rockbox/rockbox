#!/bin/bash
# hibyos_nativepatcher.sh
#
# NOTE: THIS SCRIPT IS NOT TOLERANT OF WHITESPACE IN FILENAMES OR PATHS

usage="hibyos_nativepatcher.sh
    
    USAGE: 
    
    hibyos_nativepatcher.sh <mkrbinstall/mkstockuboot> [arguments depend on mode, see below]

    hibyos_nativepatcher.sh mkrbinstall <OFVERNAME (erosq or eros_h2)>
        <path/to/output> <path/to/bootloader.erosq> <HWVER (hw1hw2, hw3, hw4)>

        Output file will be path/to/output/erosqnative_RBVER-HWVER-OFVERNAME.upt.
        Only the Hifiwalker H2 v1.3 uses "eros_h2", everything else uses "erosq".

    hibyos_nativepatcher.sh mkstockuboot <path/to/OFupdatefile.upt>

        Output file will be path/to/OFupdatefile-rbuninstall.upt."

# check OS type and for any needed tools
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

# make sure we can find patch_manifest
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
if !(which $SCRIPT_DIR/patch_manifest.pl > /dev/null); then
    echo "couldn't find patch_manifest.pl!"
    exit 1
fi

###########################################################################
# MKRBINSTALL
###########################################################################
if [[ "$1" == "mkrbinstall" ]]; then
    echo "Creating installation image from bootloader file..."

    # make sure all arguments are accounted for...
    if [[ -z "$5" ]]; then
        echo "not all parameters included, please see usage:"
        echo "$usage"
        exit 1
    fi

    # validate arguments
    outputdir=$(realpath "$3")
    if !(ls "$outputdir" >& /dev/null); then
        echo "directory $outputdir doesn't seem to exist. Please make sure it exists, then re-run hibyos_nativepatcher.sh."
        exit 1
    fi

    # note, bootloaderfile might still be a valid path, but not a valid bootloader file... check to make sure tar can extract it okay.
    bootloaderfile=$(realpath "$4")
    if !(ls "$bootloaderfile" >& /dev/null); then
        echo "bootloader file $bootloaderfile doesn't seem to exist. Please make sure it exists, then re-run hibyos_nativepatcher.sh."
        exit 1
    fi

    # make working directory...
    mkdir "$outputdir"/working_dir
    workingdir=$(realpath "$outputdir"/working_dir)
    mkdir "$workingdir"/bootloader

    # extract bootloader file
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macos
        tar -xvf "$bootloaderfile" --cd "$workingdir"/bootloader
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        # linux-gnu
        tar -xvf "$bootloaderfile" -C "$workingdir"/bootloader
    fi

    # make sure we got what we wanted
    if !(ls "$workingdir"/bootloader/bootloader.ucl >& /dev/null); then
        echo "can't find bootloader.ucl! help!"
        rm -rf "$workingdir"
        exit 1
    elif !(ls "$workingdir"/bootloader/spl.erosq >& /dev/null); then
        echo "can't find spl.erosq! help!"
        rm -rf "$workingdir"
        exit 1
    fi

    bootver=$(cat "$workingdir"/bootloader/bootloader-info.txt)
    if [ -z "$bootver" ]; then
        echo "COULDN'T FIND BOOTLOADER-INFO!"
        rm -rf "$workingdir"
        exit 1
    fi

    # if uboot.bin already exists, something is weird.
    if (ls "$workingdir"/image_contents/uboot.bin >& /dev/null); then
        echo "$workingdir/image_contents/uboot.bin already exists, something went weird."
        rm -rf "$workingdir"
        exit 1
    fi

    # everything exists, make the bin
    mkdir "$workingdir"/image_contents/
    touch "$workingdir"/image_contents/uboot.bin
    echo "PATCHING!"
    dd if="$workingdir"/bootloader/spl.erosq of="$workingdir"/image_contents/uboot.bin obs=1 seek=0 conv=notrunc
    dd if="$workingdir"/bootloader/bootloader.ucl of="$workingdir"/image_contents/uboot.bin obs=1 seek=26624 conv=notrunc

    # create update.txt
    md5=($(md5sum "$workingdir"/image_contents/uboot.bin))
    if [ -z "$md5" ]; then
        echo "COULDN'T MD5SUM UBOOT.BIN!"
        rm -rf "$workingdir"
        exit 1
    fi
    echo "Create update manifest with md5sum $md5"
    echo "" > "$workingdir"/image_contents/update.txt
    "$SCRIPT_DIR"/patch_manifest.pl "$md5" "$workingdir"/image_contents/update.txt

    # create version.txt
    echo "version={
        name=$2
        ver=2024-09-10T14:42:18+08:00
}" > "$workingdir"/image_contents/version.txt

    outputfilename="erosqnative_$bootver-$5-$2"


###########################################################################
# MKSTOCKUBOOT
###########################################################################
elif [[ "$1" == "mkstockuboot" ]]; then
    echo "Creating uninstallation image from stock update image..."

    # make sure all arguments are accounted for...
    if [[ -z "$2" ]]; then
        echo "not all parameters included, please see usage:"
        echo "$usage"
        exit 1
    fi

    updatefile=$(realpath "$2")
    updatefile_path=$(dirname "$updatefile")
    updatefile_name=$(basename "$updatefile")
    updatefile_name_noext=$(echo "$updatefile_name" | perl -ne "s/\.\w*$// && print")
    outputdir="$updatefile_path"
    outputfilename="$updatefile_name_noext"-rbuninstall

    mkdir "$updatefile_path"/working_dir
    workingdir=$(realpath "$updatefile_path"/working_dir)

    # copy update.upt to update.iso
    cp "$updatefile" "$workingdir"/"$updatefile_name_noext"-cpy.iso

    mkdir "$workingdir"/image_contents

    # extract iso
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macos
        hdiutil attach "$workingdir"/"$updatefile_name_noext"-cpy.iso -mountpoint "$workingdir"/contentsiso

        # copy out iso contents
        cp "$workingdir"/contentsiso/* "$workingdir"/image_contents

        # unmount iso
        hdiutil detach "$workingdir"/contentsiso
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        # linux-gnu
        7z -o"$workingdir"/image_contents x "$workingdir"/"$updatefile_name_noext"-cpy.iso
    fi

    chmod 777 "$workingdir"/image_contents/*

    # modify update.txt
    md5=($(md5sum "$workingdir"/image_contents/uboot.bin))
    if [ -z "$md5" ]; then
        echo "COULDN'T MD5SUM UBOOT.BIN!"
        rm -rf "$working_dir"
        exit 1
    fi
    echo "add to update manifest with md5sum $md5"
    "$SCRIPT_DIR"/patch_manifest.pl "$md5" "$workingdir"/image_contents/update.txt

######################################################################
# PRINT USAGE
######################################################################
else
    echo "$usage"
    exit 1
fi

######################################################################
# Common: make the image
######################################################################
# make the image
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macos
    hdiutil makehybrid -iso -joliet -o "$outputdir"/output.iso "$workingdir"/image_contents/
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # linux-gnu
    genisoimage -o "$outputdir"/output.iso "$workingdir"/image_contents/
fi

# rename
mv "$outputdir"/output.iso "$outputdir"/"$outputfilename".upt

# cleaning up
rm -rf "$workingdir"

exit 0
