#!/usr/bin/perl -w
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Original Python Copyright © 2010-2011 Rafaël Carré <rafael.carre@gmail>
# Perl port Copyright © 2026 Solomon Peachy <pizza@shaftnet.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
use strict;
use IPC::Open3;

my $bypass = 0;
my $need_interwork = 1;

# Walk arguments, looking for problematic stuff
for (my $i = 0; $i < scalar(@ARGV); $i++) {
    my $arg = $ARGV[$i];

    # Bypass if not actually compiling
    if ($arg eq '-E' || $arg eq '-MM' || $arg eq '-v' || $arg eq '--version') {
        $bypass = 1;
        last;
    }
    # Bypass if output doesn't end in .o
    if ($arg eq '-o' && $i < (scalar(@ARGV) - 1)) {
	if (substr($ARGV[$i+1], -2) ne '.o') {
	    $bypass = 1;
	}
	last;
    }

    if ($arg eq '-mthumb-interwork') {
	$need_interwork = 0;
    }
}

if ($bypass) {
    exec(@ARGV);
}

# thumb-interwork is required; add if not present
if ($need_interwork) {
    push(@ARGV, '-mthumb-interwork');
}

# First, try to build it in thumb mode
push(@ARGV, '-mthumb');

my $pid = open3(*IN, *OUT, *ERR, @ARGV);
close(*IN);
close(*OUT);
binmode(*ERR, ':encoding(utf8)');
my (@err);
while (<ERR>) {
    push @err, $_;
}
close(*ERR);
waitpid($pid, 0);

if ($? >> 8) {
    # Thumb failed, build in ARM mode
    pop(@ARGV);
    exec(@ARGV);
} else {
    # Flush stdout
    foreach (@err) {
	print STDERR $_;
    }
}
