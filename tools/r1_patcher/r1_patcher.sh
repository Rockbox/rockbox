#!/bin/bash

#Prerequisites:
#sudo apt update && sudo apt install -y p7zip-full squashfs-tools genisoimage coreutils

# Fail fast on any error, undefined variable, or failed pipeline
set -euo pipefail

# Report errors with line number and the failing command
trap 'echo "ERROR at line ${LINENO}: ${BASH_COMMAND}" >&2' ERR

usage() {
    echo 'Usage:' >&2
    echo '  ./r1_patcher.sh r1.upt bootloader.r1' >&2
    echo '' >&2
    echo 'Advanced usage:' >&2
    echo '  ./r1_patcher.sh --unpack r1.upt [working_dir]' >&2
    echo '  ./r1_patcher.sh --inject-app bootloader.r1 [working_dir]' >&2
    echo '  ./r1_patcher.sh --pack [working_dir] [output.upt]' >&2
    exit 1
}

mode=""
if [[ $# -ge 1 && "$1" == --* ]]; then
    mode="$1"
    shift
fi

case "$mode" in
    "")
        if [[ $# -ne 2 ]]; then
            usage
        fi
        updatefile="$1"
        bootloader="$2"
        workingdir="$(realpath -m ./working_dir)"
        updatefile_rb="${updatefile%.*}_rb.upt"
        do_unpack=1
        do_inject=1
        do_pack=1
        do_cleanup=1
        ;;
    --unpack)
        if [[ $# -lt 1 || $# -gt 2 ]]; then
            usage
        fi
        updatefile="$1"
        workingdir="${2:-./working_dir}"
        workingdir="$(realpath -m "$workingdir")"
        do_unpack=1
        do_inject=0
        do_pack=0
        do_cleanup=0
        ;;
    --inject-app)
        if [[ $# -lt 1 || $# -gt 2 ]]; then
            usage
        fi
        bootloader="$1"
        workingdir="${2:-./working_dir}"
        workingdir="$(realpath -m "$workingdir")"
        do_unpack=0
        do_inject=1
        do_pack=0
        do_cleanup=0
        ;;
    --pack)
        if [[ $# -lt 1 || $# -gt 2 ]]; then
            usage
        fi
        workingdir="${1:-./working_dir}"
        workingdir="$(realpath -m "$workingdir")"
        if [[ $# -ge 2 ]]; then
            updatefile_rb="$2"
        else
            updatefile_rb="$(basename "$workingdir")_rb.upt"
        fi
        do_unpack=0
        do_inject=0
        do_pack=1
        do_cleanup=0
        ;;
    *)
        usage
        ;;
esac

currentdir="$(pwd)"

if [[ -n "${updatefile_rb:-}" && "$updatefile_rb" != /* ]]; then
    updatefile_rb="$currentdir/$updatefile_rb"
fi

################################################################################
### init
################################################################################

workingdir_in="$workingdir/in"
workingdir_out="$workingdir/out"

if [[ "$do_unpack" -eq 1 ]]; then
    rm -rf "$workingdir"

    mkdir "$workingdir"
    mkdir "$workingdir_in"
    mkdir "$workingdir_out"
fi

################################################################################
### extract
################################################################################

if [[ "$do_unpack" -eq 1 ]]; then
    # extract iso
    mkdir -p "$workingdir_in/image_contents"
    7z -o"$workingdir_in/image_contents" x "$updatefile"

    # create xImage (unchanged)
    mkdir -p "$workingdir_out/xImage"
    cat "$workingdir_in/image_contents/ota_v0"/xImage.* > "$workingdir_out/xImage/xImage"

    # create rootfs
    mkdir -p "$workingdir_in/rootfs"
    mkdir -p "$workingdir_in/rootfs/extracted"
    cat "$workingdir_in/image_contents/ota_v0"/rootfs.squashfs.* > "$workingdir_in/rootfs/rootfs.squashfs"

    # extract rootfs
    unsquashfs -f -d "$workingdir_in/rootfs/extracted" "$workingdir_in/rootfs/rootfs.squashfs"
fi

################################################################################
### update
################################################################################

if [[ "$do_inject" -eq 1 ]]; then
    # copy 'bootloader'
    bootloader_file="bootloader.rb"
    cp "$bootloader" "$workingdir_in/rootfs/extracted/usr/bin/$bootloader_file"
    chmod 0755 "$workingdir_in/rootfs/extracted/usr/bin/$bootloader_file"

    # create modified 'hiby_player.sh' script
    cat << EOF > "$workingdir_in/rootfs/extracted/usr/bin/hiby_player.sh"
#!/bin/sh

killall    hiby_player    &>/dev/null
killall -9 hiby_player    &>/dev/null

killall    $bootloader_file    &>/dev/null
killall -9 $bootloader_file    &>/dev/null

/usr/bin/$bootloader_file
sleep 1s
EOF
    chmod 0755 "$workingdir_in/rootfs/extracted/usr/bin/hiby_player.sh"
fi

################################################################################
### Rockbox Hotplug Logic
################################################################################

if [[ "$do_inject" -eq 1 ]]; then
    # 1. Create hotplug helper script
    cat << 'EOF' > "$workingdir_in/rootfs/extracted/usr/bin/rb_hotplug.sh"
#!/bin/sh
MNT_SD="/data/mnt/sd_0"
MNT_USB="/data/mnt/usb"

case "$MDEV" in
    mmcblk*) MNT="$MNT_SD" ;;
    sd*)     MNT="$MNT_USB" ;;
    *)       exit 0 ;;
esac

case "$ACTION" in
    add|"")
        mkdir -p "$MNT"
        mount -t auto -o sync,noatime "/dev/$MDEV" "$MNT"
        ;;
    remove)
        umount -l "$MNT"
        ;;
esac
EOF
    chmod 0755 "$workingdir_in/rootfs/extracted/usr/bin/rb_hotplug.sh"

    # 2. Check mdev.conf and append rules only if missing
    MDEV_CONF="$workingdir_in/rootfs/extracted/etc/mdev.conf"

    if ! grep -q "mmcblk" "$MDEV_CONF"; then
        echo "mmcblk[0-9]p[0-9] 0:0 660 */usr/bin/rb_hotplug.sh" >> "$MDEV_CONF"
    fi

    if ! grep -q "sd\[a-z\]" "$MDEV_CONF"; then
        echo "sd[a-z][0-9]      0:0 660 */usr/bin/rb_hotplug.sh" >> "$MDEV_CONF"
    fi
fi

################################################################################
### rebuild
################################################################################

if [[ "$do_pack" -eq 1 ]]; then
    mkdir -p "$workingdir_out/image_contents/ota_v0"

    rm -f "$workingdir_out/rootfs.squashfs"
    mksquashfs "$workingdir_in/rootfs/extracted" "$workingdir_out/rootfs.squashfs" -comp lzo -all-root -noappend

    cd "$workingdir_out/image_contents/ota_v0"

    # rootfs.squashfs
    split -b 512k "$workingdir_out/rootfs.squashfs" --numeric-suffixes=0 -a 4 rootfs.squashfs.

    rootfs_md5=($(md5sum "$workingdir_out/rootfs.squashfs"))
    rootfs_size=$(stat -c%s "$workingdir_out/rootfs.squashfs")
    md5=$rootfs_md5

    ota_md5_rootfs="ota_md5_rootfs.squashfs.$md5"

    for part in $(ls rootfs.squashfs.[0-9]* | sort); do
        md5next=($(md5sum "$part"))
        echo $md5next >> $ota_md5_rootfs
        mv "$part" "$part.$md5"
        md5=$md5next
    done

    # xImage
    split -b 512k "$workingdir_out/xImage/xImage" --numeric-suffixes=0 -a 4 xImage.

    ximage_md5=($(md5sum "$workingdir_out/xImage/xImage"))
    ximage_size=$(stat -c%s "$workingdir_out/xImage/xImage")
    md5=$ximage_md5

    ota_md5_xImage="ota_md5_xImage.$md5"

    for part in $(ls xImage.[0-9]* | sort); do
        md5next=($(md5sum "$part"))
        echo $md5next >> $ota_md5_xImage
        mv "$part" "$part.$md5"
        md5=$md5next
    done

    # ota_update.in

    echo "ota_version=0

img_type=kernel
img_name=xImage
img_size=$ximage_size
img_md5=$ximage_md5

img_type=rootfs
img_name=rootfs.squashfs
img_size=$rootfs_size
img_md5=$rootfs_md5
" > ota_update.in

    echo > ota_v0.ok

    # ota_config.in
    cd "$workingdir_out/image_contents"

    echo "current_version=0" > ota_config.in

    # iso
    genisoimage -f -U -J -joliet-long -r -allow-lowercase -allow-multidot -input-charset utf-8 -o "$updatefile_rb" "$workingdir_out/image_contents/"
fi

################################################################################
### cleanup
################################################################################

if [[ "$do_cleanup" -eq 1 ]]; then
    rm -rf "$workingdir"
else
  if [[ "$do_unpack" -eq 1 ]]; then
    echo "Extracted to: $workingdir/in/rootfs/extracted"
  fi 
fi
if [[ "$do_pack" -eq 1 ]]; then
    echo "Image file created: $updatefile_rb"
fi

exit 0
