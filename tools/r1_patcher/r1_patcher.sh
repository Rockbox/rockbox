#!/bin/bash

#Prerequisites:
#sudo apt update && sudo apt install -y p7zip-full squashfs-tools genisoimage coreutils

if [[ $# -ne 2 ]]; then
    echo 'usage: ./r1_patcher.sh r1.upt bootloader.r1' >&2
    exit 1
fi

################################################################################
### init
################################################################################

currentdir="$(pwd)"
updatefile="$(basename "$1")"
updatefile_rb="${updatefile%.*}_rb.upt"

workingdir="$(realpath ./working_dir)"
workingdir_in="$workingdir/in"
workingdir_out="$workingdir/out"

rm -rf "$workingdir"

mkdir "$workingdir"
mkdir "$workingdir_in"
mkdir "$workingdir_out"

################################################################################
### extract
################################################################################

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

################################################################################
### update
################################################################################

# copy 'bootloader'
cp "$2" "$workingdir_in/rootfs/extracted/usr/bin/bootloader.rb"
chmod 0755 "$workingdir_in/rootfs/extracted/usr/bin/bootloader.rb"

# copy modified 'hibyplayer.sh' script
cp hiby_player.sh "$workingdir_in/rootfs/extracted/usr/bin/"
chmod 0755 "$workingdir_in/rootfs/extracted/usr/bin/hiby_player.sh"

################################################################################
### Rockbox Hotplug Logic
################################################################################

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
        mount -t auto -o flush,noatime "/dev/$MDEV" "$MNT"
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

################################################################################
### rebuild
################################################################################

mkdir -p "$workingdir_out/image_contents/ota_v0"

mksquashfs "$workingdir_in/rootfs/extracted" "$workingdir_out/rootfs.squashfs" -comp lzo -all-root

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
genisoimage -f -U -J -joliet-long -r -allow-lowercase -allow-multidot -o "$currentdir/$updatefile_rb" "$workingdir_out/image_contents/"

################################################################################
### cleanup
################################################################################

rm -rf "$workingdir"

exit 0
