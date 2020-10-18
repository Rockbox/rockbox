#!/usr/bin/perl -w
##################
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
#     Hiby patcher Copyright (C) 2020 Solomon Peachy <pizza@shaftnet.org>
#
#     Licensed under the GNU GPLv2 (or later)
#
##################

# usage: hiby_patcher.pl model imagefile bootloaderfile

# need '7z', 'mkisofs', 'md5sum', 'mkfs.ubifs'
# and https://github.com/jrspruitt/ubi_reader

use strict;

use File::Basename;

my $model = $ARGV[0];
my $inname = $ARGV[1];
my $rbbname = $ARGV[2];

my $prefix = "/tmp";   # XXX mktmp
my $debug = 0;  # Set to 1 to prevent cleaning up work dirs

my $hiby = 1;  # 1 for hibyplayer-style updates, 0 for ingenic updates

#### Let's get to work

my @ubiopts;
if ($model eq 'rocker') {
    @ubiopts = ("-e", "124KiB", "-c", "1024", "-m", "2048", "-j", "8192KiB", "-U");
} elsif ($model eq 'x3ii') {
    @ubiopts = ("-e", "124KiB", "-c", "1024", "-m", "2048", "-j", "8192KiB", "-U");
} elsif ($model eq 'x20') {
    @ubiopts = ("-e", "124KiB", "-c", "1024", "-m", "2048", "-j", "8192KiB", "-U");
} elsif ($model eq 'eros_q') {
    @ubiopts = ("-e", "124KiB", "-c", "1024", "-m", "2048", "-j", "8192KiB", "-U");
} elsif ($model eq 'm3k') {
    @ubiopts = ("-e", "124KiB", "-c", "2048", "-m", "2048", "-j", "8192KiB", "-U");
    $hiby = 0;
} else {
    die ("Unknown hiby model: $model\n");
}

my $uptname = basename($inname);
my $uptnamenew = "rb-$uptname";
my $isowork = "$prefix/iso.$uptname";
my $rootfsdir = "$prefix/rootfs.$uptname";
my $ubiname;
my $ubinamenew;
my $updatename;
my $versionname;

my @sysargs;

#### Extract OF image wrapper
if ($hiby) {
    system("rm -Rf $isowork");
    mkdir($isowork) || die ("Can't create '$isowork'");
    @sysargs = ("7z", "x", "-aoa", "-o$isowork", $inname);
    system(@sysargs);

    ### figure out the rootfs image filenames
    if ( -e "$isowork/UPDATE.TXT") {
	$updatename = "$isowork/UPDATE.TXT";
    } elsif ( -e "$isowork/update.txt") {
	$updatename = "$isowork/update.txt";
    }
    if ( -e "$isowork/VERSION.TXT") {
	$versionname = "$isowork/VERSION.TXT";
    } elsif ( -e "$isowork/version.txt") {
	$versionname = "$isowork/version.txt";
    }

    open UPDATE, $updatename || die ("Can't open update.txt!");;

    my $rootfs_found = 0;
    while (<UPDATE>) {
	chomp;
	if ($rootfs_found) {
	    if (/file_path=(.*)/) {
		$ubiname = basename($1);
		last;
	    }
	} else {
	    if (/rootfs/) {
		$rootfs_found = 1;
	}
	}
    }
    close UPDATE;

    if (! -e "$isowork/$ubiname") {
	$ubiname =~ tr/[a-z]/[A-Z]/;
	die("can't locate rootfs image ($ubiname)") if (! -e "$isowork/$ubiname");
    }

    $ubiname = "$isowork/$ubiname";
} else {
    # Deconstruct original file
    # TODO.  Rough sequence:

#   unzip m3k.fw
#   for x in recovery-update/nand/update*zip ; do unzip -o $x ; done
#   rm -Rf *zip META-INF
#   parse update000/update.xml:
#    <name>system.ubi</name>
#    <type>ubifs</type>
#    <size>###</size>              ## total size, bytes
#    <chunksize>##</chunksize>     ## max for each one I guess  [optional, 0 def]
#    <chunkcount>##</chunkcount>   ## add up                    [optional, 1 def]
#
# * Track name, start block
# * when we find the first ubifs block, record name, start block #, end block #
#
#  alternatively, cat update*/$name* > combined.ubi

    $ubiname = "$uptname";
}

#### Extract RootFS
system("rm -Rf $rootfsdir");
mkdir($rootfsdir) || die ("Can't create '$rootfsdir'");

@sysargs = ("ubireader_extract_files", "-k", "-o", $rootfsdir, $ubiname);
system(@sysargs);

my $rbbasename = basename($rbbname);

#### Mangle RootFS

# Save version into rootfs
my $version = `cat rockbox-info.txt | grep Version | cut -f2 -d' '`;
open FILE, ">$rootfsdir/etc/rockbox-bl-info.txt" || die ("can't write version txt");
print FILE $version;
close FILE;

if ($hiby) {
    my $bootloader_sh =
    "#!/bin/sh

#mkdir -p /mnt/sd_0
#
#mount /dev/mmcblk0 /mnt/sd_0 &>/dev/null || \
#mount /dev/mmcblk0p1 /mnt/sd_0 &>/dev/null

killall    hiby_player    &>/dev/null
killall -9 hiby_player    &>/dev/null

killall $rbbasename       &>/dev/null
killall -9 $rbbasename    &>/dev/null

# /etc/init.d/K90adb start

# Rockbox launcher!
/usr/bin/$rbbasename
sleep 1
reboot
";
    open FILE, ">$rootfsdir/usr/bin/hiby_player.sh" || die ("can't write bootloader script!");
    print FILE $bootloader_sh;
    close FILE;
    chmod 0755, "$rootfsdir/usr/bin/hiby_player.sh";

    # Auto mount/unmount external USB drives and SD card
    open  FILE, ">>$rootfsdir/etc/mdev.conf" || die ("can't access mdev conf!");
    print FILE "sd[a-z][0-9]+ 0:0 664 @ /etc/rb_inserting.sh\n";
    print FILE "mmcblk[0-9]p[0-9] 0:0 664 @ /etc/rb_inserting.sh\n";
    print FILE "mmcblk[0-9] 0:0 664 @ /etc/rb_inserting.sh\n";
    print FILE "sd[a-z] 0:0 664 \$ /etc/rb_removing.sh\n";
    print FILE "mmcblk[0-9] 0:0 664 \$ /etc/rb_removing.sh\n";
    close FILE;

    my $insert_sh = '#!/bin/sh
# $MDEV is the device

case $MDEV in
 mmc*)
   MNT_POINT=/mnt/sd_0
   ;;
 sd*)
   MNT_POINT=/mnt/usb
   ;;
esac

if [ ! -d $MNT_POINT ];then
   mkdir $MNT_POINT
fi

mount $MDEV $MNT_POINT
';

    open FILE, ">$rootfsdir/etc/rb_inserting.sh" || die("can't write hotplug helpers!");
    print FILE $insert_sh;
    close FILE;
    chmod 0755, "$rootfsdir/etc/rb_inserting.sh";

    my $remove_sh = '#!/bin/sh
# $MDEV is the device

case $MDEV in
 mmc*)
   MNT_POINT=/mnt/sd_0
   ;;
 sd*)
   MNT_POINT=/mnt/usb
   ;;
esac

sync;
umount -f $MNT_POINT || umount -f $MDEV;
';

    open FILE, ">$rootfsdir/etc/rb_removing.sh" || die("can't write hotplug helpers!");
    print FILE $remove_sh;
    close FILE;
    chmod 0755, "$rootfsdir/etc/rb_removing.sh";

    # Deal with a nasty race condition in automount scripts
    system("perl -pni -e 's/rm -rf/#rm -Rf/;' $rootfsdir/etc/init.d/S50sys_server");
} else {
    my $bootloader_sh =
    "#!/bin/sh

killall $rbbasename       &>/dev/null
killall -9 $rbbasename    &>/dev/null

source /etc/profile

# Try to save the flash
if ! [ -L /data/userfs/app.log ] ; then
  rm -f /data/userfs/app.log
  ln -s /dev/null /data/userfs/app.log
fi

# Rockbox launcher!
/usr/bin/$rbbasename
sleep 1
reboot
";
    open FILE, ">$rootfsdir/usr/project/bin/play.sh" || die ("can't write bootloader script!");
    print FILE $bootloader_sh;
    close FILE;
    chmod 0755, "$rootfsdir/usr/project/bin/play.sh";

}

# Copy bootloader over
@sysargs=("cp", "$rbbname", "$rootfsdir/usr/bin/$rbbasename");
system(@sysargs);

### Generate new RootFS UBI image
$ubinamenew = "$ubiname.new";

@sysargs = ("mkfs.ubifs", @ubiopts, "-v", "-o", $ubinamenew, "-r", "$rootfsdir");
system(@sysargs);

system("rm -Rf $rootfsdir") if (!$debug);

if ($hiby) {
    # md5sum
    my $md5 = `md5sum $ubinamenew | cut -d ' ' -f 1`;

    system("mv $ubinamenew $ubiname");

    ### Generate new ISO9660 update image

    # Correct md5sum for new rootfs image
    open UPDATE, "<$updatename" || die ("Can't open update.txt!");
    open UPDATEO, ">$updatename.new" || die ("Can't open update.txt!");

    my $rootfs_found = 0;
    while (<UPDATE>) {
	if ($rootfs_found) {
	    if (s/md5=.*/md5=$md5/) {
		$rootfs_found=0;
	    }
	} else {
	    if (/rootfs/) {
		$rootfs_found = 1;
	    }
	}
	print UPDATEO;
    }
    close UPDATE;
    close UPDATEO;
    system("mv $updatename.new $updatename");

    # Fix up version text, if needed (AGPTek Rocker 1.31 beta)

    open UPDATE, "<$versionname" || die ("Can't open version.txt!");;
    open UPDATEO, ">$versionname.new" || die ("Can't open version.txt!");

    while (<UPDATE>) {
	s/ver=1\.0\.0\.0/ver=2018-10-07T00:00:00+08:00/;
	print UPDATEO;
    }

    close UPDATE;
    close UPDATEO;
    system("mv $versionname.new $versionname");

    @sysargs = ("mkisofs", "-volid", "CDROM", "-o", $uptnamenew, $isowork);
    system(@sysargs);

    system("rm -Rf $isowork") if (!$debug);
} else {
    # Reconstruct fiio/ingenic firmware update image
    # very TODO
}
