#!/usr/bin/perl
# add bootloader info to update manifest
# usage: ./patch_manifest.pl <md5sum> <path/to/update.txt>

my $md5 = $ARGV[0];
my $updatefile = $ARGV[1];
my $bootloader_manif =
"bootloader={
        name=uboot
        file_path=autoupdate/uboot.bin
        md5=$md5
}\n";

# read in existing manifest
open(FH, '<', "$updatefile");
read(FH, my $manifest, -s FH);
close(FH);

# delete existing bootloader entry if exists
$manifest =~ s/bootloader\s*=\s*{[^}]*}//;

# add our own bootloader entry
$manifest = "$bootloader_manif$manifest";

open(FH, '>', "$updatefile");
print FH $manifest;
close(FH);
