#!/usr/bin/perl
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: configure 13215 2007-04-20 11:58:39Z bagder $
#
# Purpose: extract and gather info from a build and put that in a standard
# way in the output file. Meant to be put in rockbox zip package to help and
# aid machine installers and more.
#

my $output = $ARGV[0];

sub filesize {
    my ($f)=@_;
    my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
        $atime,$mtime,$ctime,$blksize,$blocks)
        = stat($f);
    return $size;
}

if(!$output) {
    print "Usage: mkinfo.pl <filename>\n";
    exit;
}
open(O, ">$output") || die "couldn't open $output for writing";

printf O ("Target: %s\n", $ENV{'ARCHOS'});
printf O ("Target id: %d\n", $ENV{'TARGET_ID'});
printf O ("Target define: %s\n", $ENV{'TARGET'});
printf O ("Version: %s\n", $ENV{'VERSION'});
printf O ("Binary: %s\n", $ENV{'BINARY'});
printf O ("Binary size: %s\n", filesize($ENV{'BINARY'}));
printf O ("Actual size: %s\n", filesize("apps/rockbox.bin"));

my @out=`$ENV{'CC'} --version`;
chomp $out[0];
printf O ("gcc: %s\n", $out[0]);

close(O);
