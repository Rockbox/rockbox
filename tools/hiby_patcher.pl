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
#     Licensed under the GNU GPLv2 (or newer)
#
##################

# usage: hiby_patcher.pl model imagefile bootloaderfile

# need '7z', 'mkisofs', 'md5sum', 'mkfs.ubifs'
# and https://github.com/jrspruitt/ubi_reader

# Rocker: PEB size 128KiB, I/O 2048, LEBs size: 124KiB, max LEBs 1024 (?)

use strict;

use File::Basename;

my $model = $ARGV[0];
my $inname = $ARGV[1];
my $rbbname = $ARGV[2];

my $prefix = "/tmp";   # XXX mktmp
my $debug = 0;  # Set to 1 to prevent cleaning up work dirs

#### Let's get to work

my @ubiopts;
if ($model eq 'rocker') {
    @ubiopts = ("-e", "124KiB", "-c", "1024", "-m", "2048", "-j", "8192KiB", "-U");
} elsif ($model eq 'x3ii') {
    @ubiopts = ("-e", "124KiB", "-c", "1024", "-m", "2048", "-j", "8192KiB", "-U");
} elsif ($model eq 'x20') {
    @ubiopts = ("-e", "124KiB", "-c", "1024", "-m", "2048", "-j", "8192KiB", "-U");
} else {
    die ("Unknown hiby model: $model\n");
}

my $uptname = basename($inname);
my $uptnamenew = "rb-$uptname";
my $isowork = "$prefix/iso.$uptname";
my $rootfsdir = "$prefix/rootfs.$uptname";
my $ubiname;
my $ubinamenew;


my @sysargs;


### Extract outer ISO9660 image
system("rm -Rf $isowork");
mkdir($isowork) || die ("Can't create '$isowork'");
@sysargs = ("7z", "x", "-aoa", "-o$isowork", $inname);
system(@sysargs);

### figure out the rootfs image filename
my $updatename;
if ( -e "$isowork/UPDATE.TXT") {
    $updatename = "$isowork/UPDATE.TXT";
} elsif ( -e "$isowork/update.txt") {
    $updatename = "$isowork/update.txt";
}
open UPDATE, $updatename || die ("Can't open update.txt!");;

my $rootfs_found = 0;
while (<UPDATE>) {
    chomp;
    if ($rootfs_found) {
	if (/file_path=(.*)/) {
	    $ubiname = basename($1);
	    $ubiname =~ tr/[a-z]/[A-Z]/;
	    last;
	}
    } else {
	if (/rootfs/) {
	    $rootfs_found = 1;
	}
    }
}
close UPDATE;

die("can't locate rootfs image") if (! -e "$isowork/$ubiname");
$ubiname = "$isowork/$ubiname";

### Extract RootFS
system("rm -Rf $rootfsdir");
mkdir($rootfsdir) || die ("Can't create '$rootfsdir'");

@sysargs = ("ubireader_extract_files", "-k", "-o", $rootfsdir, $ubiname);
system(@sysargs);

### Mangle RootFS

# Generate rb_bootloader.sh
my $rbbasename = basename($rbbname);
my $bootloader_sh =
    "#!/bin/sh

mount /dev/mmcblk0 /mnt/sd_0 &>/dev/null || \
mount /dev/mmcblk0p1 /mnt/sd_0 &>/dev/null

killall $rbbasename
killall -9 $rbbasename

/usr/bin/$rbbasename
sleep 1
reboot
    ";
open FILE, ">$rootfsdir/usr/bin/hiby_player.sh" || die ("can't write bootloader script!");
print FILE $bootloader_sh;
close FILE;

# Copy bootloader over
@sysargs=("cp", "$rbbname", "$rootfsdir/usr/bin/$rbbasename");
system(@sysargs);

### Generate new RootFS UBI image
$ubinamenew = "$ubiname.new";

@sysargs = ("mkfs.ubifs", @ubiopts, "-v", "-o", $ubinamenew, "-r", "$rootfsdir");
system(@sysargs);

system("rm -Rf $rootfsdir") if (!$debug);

# md5sum
my $md5 = `md5sum $ubinamenew | cut -d ' ' -f 1`;

system("mv $ubinamenew $ubiname");

### Generate new ISO9660 update image

# Update version string as needed  XXX

open UPDATE, "<$updatename" || die ("Can't open update.txt!");;
open UPDATEO, ">$updatename.new" || die ("Can't open update.txt!");;

$rootfs_found = 0;
while (<UPDATE>) {
    if ($rootfs_found) {
	if (s/md5=.*/md5=$md5/) {
	    print "#### $_ ####\n";
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
@sysargs = ("mkisofs", "-volid", "CDROM", "-o", $uptnamenew, $isowork);
system(@sysargs);

system("rm -Rf $isowork") if (!$debug);
