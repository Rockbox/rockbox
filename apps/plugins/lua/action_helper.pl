#!/usr/bin/env perl
############################################################################
#             __________               __   ___.                  
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/ 
# $Id$
#
# Copyright (C) 2009 by Maurus Cuelenaere
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
############################################################################

$i = 0;
$j = 0;
while(my $line = <STDIN>)
{
    chomp($line);
    if($line =~ /^\s*(ACTION_[^\s]+)(\s*=.*)?,\s*$/)
    {
        $actions[$i] = sprintf("\t%s = %d,\n", $1, $i);
        $i++;
    }
    elsif($line =~ /^\s*(PLA_[^\s]+)(\s*=.*)?,\s*$/)
    {
        # PLA_* begins at LAST_ACTION_PLACEHOLDER+1, thus i+1
        $actions[$i] = sprintf("\t%s = %d,\n", $1, $i+1);
        $i++;
    }
    elsif($line =~ /^\s*(CONTEXT_[^\s]+)(\s*=.*)?,\s*$/)
    {
        $contexts[$j] = sprintf("\t%s = %d,\n", $1, $j);
        $j++;
    }
}

print "-- Don't change this file!\n";
printf "-- It is automatically generated of action.h %s\n", '$Revision$';

print "rb.actions = {\n";
foreach $action(@actions)
{
    print $action;
}
print "}\n";

print "rb.contexts = {\n";
foreach $context(@contexts)
{
    print $context;
}
print "}\n";
