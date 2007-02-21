#!/usr/bin/perl

# This script creates the h100sums.h and h300sums.h files for fwpatcher.
#
# It expects a file tree with scrambled and descrambled
# firmwares like this:
# orig-firmware/
#               h1xx/
#                    1.66jp/
#                           ihp_100.bin
#                           ihp_100.hex
#                           ihp_120.bin
#                           ihp_120.hex
#               h3xx/
#                    1.29jp/
#                           H300.bin
#                           H300.hex
# etc.
#
# It also expects the bootloader binaries in the current directory:
# bootloader-h100.bin
# bootloader-h120.bin
# bootloader-h300.bin

$orig_path = "~/orig-firmware";

mksumfile("100");
mksumfile("120");
mksumfile("300");

sub mksumfile {
    ($model) = @_;

    open FILE, ">h${model}sums.h" or die "Can't open h${model}sums.h";

    print FILE "/* Checksums of firmwares for ihp_$model */\n";
    print FILE "/* order: unpatched, patched */\n\n";

    if($model < 300) {
        foreach("1.63eu","1.63k", "1.63us", "1.65eu","1.65k", "1.65us",
		"1.66eu", "1.66k", "1.66us", "1.66jp") {
            `../mkboot $orig_path/h1xx/$_/ihp_$model.bin bootloader-h$model.bin ihp_$model.bin`;
            `../scramble -iriver ihp_$model.bin ihp_$model.hex`;
            $origsum = `md5sum $orig_path/h1xx/$_/ihp_$model.hex`;
            chomp $origsum;
            ($os, $or) = split / /, $origsum;
            $sum = `md5sum ihp_$model.hex`;
            chomp $sum;
            ($s, $r) = split / /, $sum;
            print FILE "/* $_ */\n";
            print FILE "{\"$os\", \"$s\"},\n";
        }
    } else {
        foreach("1.28eu", "1.28k", "1.28jp", "1.29eu", "1.29k", "1.29jp",
		"1.30eu") {
            `../mkboot -h300 $orig_path/h3xx/$_/H$model.bin bootloader-h$model.bin H$model.bin`;
            `../scramble -iriver H$model.bin H$model.hex`;
            $origsum = `md5sum $orig_path/h3xx/$_/H$model.hex`;
            chomp $origsum;
            ($os, $or) = split / /, $origsum;
            $sum = `md5sum H$model.hex`;
            chomp $sum;
            ($s, $r) = split / /, $sum;
            print FILE "/* $_ */\n";
            print FILE "{\"$os\", \"$s\"},\n";
        }
    }
    close FILE;
}
