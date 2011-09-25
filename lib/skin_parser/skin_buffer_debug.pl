#!/usr/bin/perl
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

%allocs = ();

while (<>) {
    if (/([0-9]*) (.*)$/) {
        $key = $2;
        $value = $1;
        if (exists $allocs{$key}) {
            $val = $allocs{$key}[0];
            $count = $allocs{$key}[1];
            $allocs{$key} = [$value + $val, $count+1]
        } else {
            $allocs{$key} = [$value, 1]
        }
    }
}
print "Count\tTotal cost\tLine\n";
for my $key ( keys %allocs ) {
            $val = $allocs{$key}[0];
            $count = $allocs{$key}[1];
        print "$count\t$val\t$key\n";
}
