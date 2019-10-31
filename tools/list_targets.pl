#!/usr/bin/perl
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# List all targets in builds.pm, categorized by target status.

require "./builds.pm";

print "Stable:\n";

for my $b (&stablebuilds) {
    print $builds{$b}{name} , "\n";
}

print "Unstable:\n";
for my $b (&usablebuilds) {
    print $builds{$b}{name} , "\n";
}

print "Unusable:\n";
for my $b (&allbuilds) {
        print $builds{$b}{name} , "\n" if($builds{$b}{status} == 1);
}
