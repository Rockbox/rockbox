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
# Copyright (C) 2021 by William Wilgus
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
############################################################################
#expects -dM -E source input on STDIN
use strict;
use warnings;
my $svnrev = '$Revision$';
my @buttons = ();
my $count = 1; #null sentinel
my $val;
my $def;
my $len_max_button = 0;
while(my $line = <STDIN>)
{
    chomp($line);
    if($line =~ /^#define (BUTTON_[^\s]+) (.+)$/)
    {
        $def = "{\"$1\", $2},\n";
        my $slen = length($1) + 1; # NULL terminator
        if ($slen > $len_max_button) { $len_max_button = $slen; }
        $val = $2;
        if($val =~ /^0/)
        {
            $val = oct($val)
        }
        else
        {
            $val = 0xFFFFFFFF; #only used for sorting
        }
        push(@buttons, {'name' => $1, 'value' => $val, 'def' => $def});
        $count = $count + 1;
    }
}
my @sorted = sort { @$a{'value'} <=> @$b{'value'} } @buttons;
print <<EOF
/* Don't change this file! */
/* It is automatically generated of button.h */
#include "plugin.h"
#include "button.h"
#include "button_helper.h"

const size_t button_helper_maxbuffer = $len_max_button;

static const struct available_button buttons[$count] = {
EOF
;
$count--; # don't count the sentinel
foreach my $button (@sorted)
{
    printf "    %s", @$button{'def'};
}

print <<EOF
    {"\\0", 0} /* sentinel */
};
const int available_button_count = $count;
const struct available_button * const available_buttons = buttons;

int get_button_names(char *buf, size_t bufsz, unsigned long button)
{
    int len = 0;
    buf[0] = '\\0';
    const struct available_button *btn = buttons;
    while(btn->name[0] != '\\0')
    {
        if(btn->value == 0)
        {
            if (button == 0)
            {
                buf[0] = '\\0';
                len = rb->strlcat(buf, btn->name, bufsz);
                return len;
            }
        }
        else if ((button & btn->value) == btn->value)
        {
            if (len > 0)
                rb->strlcat(buf, " | ", bufsz);
            len = rb->strlcat(buf, btn->name, bufsz);
        }
        btn++;
    }
    return len;
}
EOF
;
