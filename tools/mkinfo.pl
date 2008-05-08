#!/usr/bin/perl
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
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

sub cmd1line {
    my ($c)=@_;
    my @out=`$c 2>/dev/null`;
    chomp $out[0];
    return $out[0];
}

sub mapscan {
    my ($f)=@_;
    my $start, $end;
    open(M, "<$f");
    while(<M>) {
        if($_ =~ / +0x([0-9a-f]+) *_end = \./) {
            $end = $1;
            last;
        }
        elsif($_ =~ / +0x([0-9a-f]+) *_loadaddress = \./) {
            $start = $1;
        }
    }
    close(M);

    # return number of bytes
    return hex($end) - hex($start);
}

sub features {
    my ($f)=@_;
    my $feat;
    open(M, "<$f");
    while(<M>) {
        chomp;
        if($feat) {
            $feat.=":";
        }
        $feat.=$_;
    }
    close(M);
    return $feat;
}

if(!$output) {
    print "Usage: mkinfo.pl <filename>\n";
    exit;
}
open(O, ">$output") || die "couldn't open $output for writing";

# Variables identifying the target, that should remain the same as long
# as the hardware is unmodified
printf O ("Target: %s\n", $ENV{'MODELNAME'});
printf O ("Target id: %d\n", $ENV{'TARGET_ID'});
printf O ("Target define: %s\n", $ENV{'TARGET'});
printf O ("Memory: %d\n", $ENV{'MEMORYSIZE'});
printf O ("CPU: %s\n", $ENV{'CPU'});
printf O ("Manufacturer: %s\n", $ENV{'MANUFACTURER'});

# Variables identifying Rockbox and bootloader properties. Possibly changing
# every software upgrade.
printf O ("Version: %s\n", $ENV{'VERSION'});
printf O ("Binary: %s\n", $ENV{'BINARY'});
printf O ("Binary size: %s\n", filesize($ENV{'BINARY'}));
printf O ("Actual size: %s\n", filesize("apps/rockbox.bin"));
printf O ("RAM usage: %s\n", mapscan("apps/rockbox.map"));
printf O ("Features: %s\n", features("apps/features"));

# Variables identifying tool and build environment details
printf O ("gcc: %s\n", cmd1line("$ENV{'CC'} --version"));
printf O ("ld: %s\n", cmd1line("$ENV{'LD'} --version"));
printf O ("Host gcc: %s\n", cmd1line("$ENV{'HOSTCC'} --version"));
printf O ("Host system: %s\n", $ENV{'UNAME'});

close(O);
