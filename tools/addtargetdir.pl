#!/usr/bin/perl
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 19082 2008-11-10 23:54:24Z zagor $
#

# addtargetdir.pl - Adds target directory to gcc-generated dependency data

use File::Basename;

my $rbroot = $ARGV[0];
my $builddir = $ARGV[1];

for (<STDIN>) {
    if (/^([^:]+): (\S+) (.*)/) {
        my ($target, $src, $rest) = ($1, $2, $3);
        my $dir = dirname $src;
        $dir =~ s/$rbroot//;
        print "$builddir$dir/$target: $src $rest\n";
    }
    else {
        print $_;
    }
}
