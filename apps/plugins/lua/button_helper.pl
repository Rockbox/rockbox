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

$svnrev = '$Revision$';

print <<EOF
#include <stdio.h>
#include <stdbool.h>
#include "button.h"

struct button
{
    char* name;
    unsigned long value;
};

static struct button buttons[] = {
EOF
;

while(my $line = <STDIN>)
{
    chomp($line);
    if($line =~ /^#define (BUTTON_[^\s]+) (.+)$/)
    {
        printf "{\"%s\", %s},\n", $1, $2;
    }
}

print <<EOF
{"BUTTON_REL", BUTTON_REL},
{"BUTTON_REPEAT", BUTTON_REPEAT},
{"BUTTON_TOUCHSCREEN", BUTTON_TOUCHSCREEN},
};

int main(void)
{
    unsigned int i;
    printf("-- Don't change this file!\\n");
    printf("-- It is automatically generated of button.h \%s\\n", "$svnrev");
    printf("rb.buttons = {\\n");
    for(i=0; i<sizeof(buttons)/sizeof(struct button); i++)
        printf("\\t\%s = \%ld,\\n", buttons[i].name, buttons[i].value);
    printf("}\\n");

    return 0;
}

EOF
;
