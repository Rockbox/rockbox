#!/usr/bin/env perl
############################################################################
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $action_helper$
#
# Copyright (C) 2021 William Wilgus
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
############################################################################
#expects -E source input on STDIN
use strict;
use warnings;

my @actions = ();
my @contexts = ();
my @action_offset = ();
my @context_offset = ();
my $action_ct = 0;
my $context_ct = 0;
my $len_max_action = 0;
my $len_max_context = 0;
my $len_min_action = -1;
my $len_min_context = -1;
while(my $line = <STDIN>)
{
    chomp($line);
        if($line =~ /^\s*(ACTION_[^\s]+)(\s*=.*)?,\s*$/)
        {
            $actions[$action_ct] = $1;
            $action_ct++;
        }
        elsif($line =~ /^\s*(LAST_ACTION_PLACEHOLDER)(\s*=.*)?,\s*$/)
        { #special case don't save actual name
            $actions[$action_ct] = "";
            $action_ct++;
        }
        elsif($line =~ /^\s*(PLA_[^\s]+)(\s*=.*)?,\s*$/)
        {
            $actions[$action_ct] = $1;
            $action_ct++;
        }
        elsif($line =~ /^\s*(CONTEXT_[^\s]+)(\s*=.*)?,\s*$/)
        {
            $contexts[$context_ct] = $1;
            $context_ct++;
        }
}

print <<EOF
/* Don't change this file! */
/* It is automatically generated of action.h */
#include "plugin.h"
#include "action_helper.h"
EOF
;
#dump actions
my $offset = 0;
print "static const char action_names[]= \n";
for(my $i = 0; $i < $action_ct; $i++){
    my $act = $actions[$i];
    $act =~ s/ACTION_USB_HID_/%s/ig;  # strip the common part
    $act =~ s/ACTION_/%s/ig;  # strip the common part
    my $actlen = length($act);
    if ($actlen < $len_min_action or $len_min_action == -1){
        $len_min_action = $actlen;
    }
    if ($actions[$i] ne $act){
        printf "/*%s*/\"%s\\0\"\n", substr($actions[$i], 0, -($actlen - 2)), $act;
    } else {
        print "\"$act\\0\" \n";
    }
    my $slen = length($actions[$i]) + 1; #NULL terminator
    if ($slen > $len_max_action) { $len_max_action = $slen; }
    push(@action_offset, {'name' => $actions[$i], 'offset' => $offset});
    $offset += length($act) + 1; # NULL terminator
}
printf "\"\";/* %d + \\0 */\n\n", $offset;
@actions = ();

#dump contexts
$offset = 0;
print "static const char context_names[]= \n";
for(my $i = 0; $i < $context_ct; $i++){
    my $ctx = $contexts[$i];
    $ctx =~ s/CONTEXT_/%s/ig;  # strip the common part
    my $ctxlen = length($ctx);

    if ($ctxlen < 5){
        $ctx = $contexts[$i];
        $ctxlen = length($ctx);
    }

    if ($ctxlen < $len_min_context or $len_min_context == -1){
        $len_min_context = $ctxlen;
    }
    if ($contexts[$i] ne $ctx){
        printf "/*%s*/\"%s\\0\"\n", substr($contexts[$i], 0, -($ctxlen - 2)), $ctx;
    } else {
        print "\"$ctx\\0\" \n";
    }
    my $slen = length($contexts[$i]) + 1; # NULL terminator
    if ($slen > $len_max_context) { $len_max_context = $slen; }
    push(@context_offset, {'name' => $contexts[$i], 'offset' => $offset});
    $offset += length($ctx) + 1; # NULL terminator
}
printf "\"\";/* %d + \\0 */\n\n", $offset;
@contexts = ();

printf "#define ACTION_CT %d\n", $action_ct;
print "static const uint16_t action_offsets[ACTION_CT] = {\n";
foreach my $define (@action_offset)
{
    printf("%d, /*%s*/\n", @$define{'offset'}, @$define{'name'});
}
print "};\n\n";
@action_offset = ();

printf "#define CONTEXT_CT %d\n", $context_ct;
print "#if 0 /* context_names is small enough to walk the string instead */\n";
print "static const uint16_t context_offsets[CONTEXT_CT] = {\n";
foreach my $define (@context_offset)
{
    printf("%d, /*%s*/\n", @$define{'offset'}, @$define{'name'});
}
print "};\n#endif\n\n";
@context_offset = ();

printf "#define ACTIONBUFSZ %d\n", $len_max_action;
printf "#define CONTEXTBUFSZ %d\n\n", $len_max_context;

if ($len_max_action > $len_max_context)
{
    print "const size_t action_helper_maxbuffer = ACTIONBUFSZ;\n";
    print "static char name_buf[ACTIONBUFSZ];\n";
}
else
{
    print "const size_t action_helper_maxbuffer = CONTEXTBUFSZ;\n";
    print "static char name_buf[CONTEXTBUFSZ];\n";
}
print <<EOF

char* action_name(int action)
{
    if (action >= 0 && action < ACTION_CT)
    {
        uint16_t offset = action_offsets[action];
        const char *act = &action_names[offset];
        if (action < ACTION_USB_HID_FIRST)
            rb->snprintf(name_buf, ACTIONBUFSZ, act, "ACTION_");
        else
            rb->snprintf(name_buf, ACTIONBUFSZ, act, "ACTION_USB_HID_");
    }
    else
        rb->snprintf(name_buf, ACTIONBUFSZ, "ACTION_UNKNOWN");
    return name_buf;
}

/* walk string increment offset for each NULL if desired offset found, return */
static const char *context_getoffset(int offset)
{
    const char *names = context_names;
    const size_t len = sizeof(context_names) - 1;
    int current = 0;
    if (offset > 0)
    {
        const char *pos = names;
        const char *end = names + len;
        while (pos < end)
        {
            if (*pos++ == '\\0')
            {
                current++;
                if (offset == current)
                    return pos;
                pos += $len_min_context; /* each string is at least this long */
            }
        }
    }
    return names;
}

char* context_name(int context)
{
    const char *ctx;
    if (context >= 0 && context < CONTEXT_CT)
    {
#if 0
        uint16_t offset = context_offsets[context];
        ctx = &context_names[offset];
#else
        ctx = context_getoffset(context);
#endif
    }
    else
        ctx = "%sUNKNOWN";
    rb->snprintf(name_buf, CONTEXTBUFSZ, ctx, "CONTEXT_");
    return name_buf;
}
EOF
;
