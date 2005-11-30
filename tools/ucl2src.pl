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
# Copyright (C) 2005 by Jens Arnold
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
############################################################################

if (!$ARGV[0])
{
    print <<HERE
Usage: ucl2src  [-p=<prefix>] <ucl file>

Check & strip header from an .ucl file and generate <prefix>.c and
<prefix>.h from it.
HERE
;
    exit;
}

my $prefix = $p;
if(!$prefix) {
    $prefix="uclimage";
}

my $input = $ARGV[0];
my $buffer;
my $insize;
my $readsize = 0;

open(INF, "<$input") or die "Can't open $input";
binmode INF;

# check UCL header

# magic header
read(INF, $buffer, 8);
if ($buffer ne pack("C8", 0x00, 0xe9, 0x55, 0x43, 0x4c, 0xff, 0x01, 0x1a))
{
    die "Not an UCL file.";
}
read(INF, $buffer, 4);

# method
read(INF, $buffer, 1);
if (ord($buffer) != 0x2E)
{
    die sprintf("Wrong compression method (expected 0x2E, found 0x%02X)",
                ord($buffer));
}

read(INF, $buffer, 9);

# file size
read(INF, $buffer, 4);
$insize = unpack("N", $buffer) + 8;

open(OUTF, ">$prefix.c") or die "Can't open $prefix.c";

print OUTF <<HERE
/* This file was automatically generated using ucl2src.pl */

/* Data compressed with UCL method 0x2e follows */
const unsigned char image[] = {
HERE
    ;
    
while (read(INF, $buffer, 1))
{
    $readsize++;
    printf OUTF ("0x%02x,", ord($buffer));
    if (!($readsize % 16))
    {
        print OUTF "\n";
    }
}

close(INF);

if ($readsize != $insize)
{
    die "Input file truncated, got $readsize of $insize bytes."
}

print OUTF <<HERE
};
/* end of compressed image */
HERE
    ;
close(OUTF);

open(OUTF, ">$prefix.h") or die "Can't open $prefix.h";

print OUTF "/* This file was automatically generated using ucl2src.pl */\n";
print OUTF "extern const unsigned char image[".$insize."];\n";

close(OUTF);

