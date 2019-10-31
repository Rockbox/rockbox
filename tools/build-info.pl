#!/usr/bin/perl
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Generate the build-info.release file found on download.rockbox.org

require "./builds.pm";

print "[release]\n";

foreach my $b (&stablebuilds) {
    if(exists($builds{$b}{release})) {
        print "$b=$builds{$b}{release}\n";
    }
    else {
        print "$b=$publicrelease\n";
    }
}

print "[status]\n";

foreach my $b (&allbuilds) {
    print "$b=$builds{$b}{status}\n";
}
