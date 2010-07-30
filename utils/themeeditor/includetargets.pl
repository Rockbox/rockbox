#!/usr/bin/perl

###########################################################################
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _ /  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# Copyright (C) 2010 Robert Bieber
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
############################################################################/ 

require '../../tools/builds.pm';

open(FOUT, ">targets.php");

print FOUT '<?php $targets = array(';

@keys = sort byname keys %builds;
$size = @keys;
$final = @keys[$size - 1];
for my $b (@keys) 
{
    $key = $b;
    $key =~ s/:/%:/;
    $name = $builds{$b}{name};
    $name =~ s/:/%:/;
    
    print FOUT "\"$key\"" . "=>" . '"' . $name . '"' if ($builds{$b}{status} >= 3);
    print FOUT ',' if $b ne $final && $builds{$b}{status} >= 3;
}

for my $b (@keys) 
{
    $key = $b;
    $key =~ s/:/%:/;
    $name = $builds{$b}{name};
    $name =~ s/:/%:/;
    
    print FOUT "\"$key\"" . "=>" . '"' . $name . '"' if ($builds{$b}{status} < 3);
    print FOUT ',' if $b ne $final && $builds{$b}{status} < 3;
}

print FOUT '); ?>';

close(FOUT);

