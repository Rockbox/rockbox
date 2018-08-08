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
# Copyright (C) 2018 William Wilgus
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
############################################################################

# Extracts members from c struct
# places them in a lua array 'member = {offset,size,"type"}'
# requires two passes
#  first pass outputs a c file that shall be compiled with the -S option
#  second pass extracts the member, offset, type from the assembly

my $svnrev = '$Revision$';
my $helper_name = 'LUA_RB_SETTINGS_H_HELPER';

############# configuration #############
my @sections = (
    'struct system_status',
    'struct user_settings',
    'struct replaygain_settings',
    'struct eq_band_setting',
    'struct compressor_settings',
    'struct mp3_enc_config',
);

my @sections_lua = (
    'rb.global_status',
    'rb.global_settings',
    'rb.replaygain_settings',
    'rb.eq_band_setting',
    'rb.compressor_settings',
    'rb.mp3_enc_config',
);

my $section_lua_suffix = '_offsets';

my %replace_type_prefix = (
    unsigned => 'u_',
    signed   => '',
    struct   => 's_',
    const    => 'const_',
);

my %replace_type = (
    int        => 'i',
    uint       => 'u_i',
    long       => 'l',
    char       => 'c',
    bool       => 'b',
    double     => 'd',
    '#string#' => 'str',
);

############# internal #############
my @section_count = ();  #variables found
my @section_found = ();  #{} bracket matches
my @section_lists = (); #variable declarations
my @section_asm_regex = ();
my @section_begin_regex = ();

my $header = '';
my $array_marker = '_typeisarray_';

############# precompile regex for speed #############

#type var ????;
my $decl_regex = qr/^(.+?\s.+;)/;
my $typevar_regex = qr/\W*(?<type>.*?)\W*(?<var>[^\s]+)(?<arr>)\W*;/;
my$typevar_array_regex = qr/\W*(?<type>.*?)\W*(?<var>[^\s]+)\W*(?<arr>\[.+\]).*;/;

#.."section",.."member"..=..offset,..size,..type,..arrayct
my $asm_regex = qr/.*?,.*?(\".+?\".*?=.*?,.+?,.+?\".+\".*?,.*);/;
my $asmMOST_regex = qr/\"(?<member>.+)\"=(?<offset>\d+),(?<size>\d+),\"(?<type>.+)\",(?<arr>\d+).*/;

for(my $i = 0; $i < @sections; $i++)
{
    $section_asm_regex[$i] = qr/\"$sections[$i]\"${asm_regex}/;
    $section_begin_regex[$i] = qr/^$sections[$i].*$/;
    $section_count[$i] = 0;
    $section_found[$i] = 0;
    $section_lists[$i] = '';
}

my $section_end_regex = qr/}\s*;.*$/;

#extract all the variables within the structs(sections)
while(my $line = <STDIN>)
{
    next if($line =~ /^\s+$/);

    chomp($line);

    if($header) #second pass
    {
        for(my $i = 0; $i < @sections; $i++)
        {
            if($line =~ $section_asm_regex[$i])
            {
                $section_lists[$i] .= $1.'@';
                $section_count[$i]++;
                last;
            }
        }
    }
    elsif($line =~ s/$helper_name\W*//) #is this the second pass?
    {
        $header = $line;
        #warn $header."\n";
        @section_lists = ();
        @section_count = ();
    }
    else #first pass
    {
        for(my $i = 0; $i < @sections; $i++)
        {
            if($section_found[$i] > 0)
            {
                if($line =~ $decl_regex)
                {
                    $section_lists[$i] .= $1.'@';
                    $section_count[$i]++;
                }

                if($line =~ $section_end_regex)
                {
                    $section_found[$i]--;
                }
                last;
            }
            elsif($line =~ $section_begin_regex[$i])
            {
                $section_found[$i]++;
                $section_lists[$i] = '';
                $section_count[$i] = 0;
                last;
            }
        }
    }#else
}

for(my $i = 0; $i < @sections; $i++)
{
    if($section_found[$i])
    {
        warn "$0 Warning formatting error in: $sections[$i]\n";
    }
}

sub Extract_Variable {
    #extracts the member, offset, size, and type from the include file
    my $sinput = $_[0];
    my ($type, $var, $arr);

    $sinput =~ s{\s*\*\s*}{_ptr}gx;
    if($sinput =~ $typevar_array_regex) #arrays
    {
        $type = $+{type};
        $var  = $+{var};
        $arr  = $+{var};
        if($sinput =~ s/\schar\s//){$type = $replace_type{'#string#'};}
        else{$type .= ${array_marker};} #for identification of array .. stripped later
    }
    elsif($sinput =~ $typevar_regex)
    {
        $type = $+{type};
        $var  = $+{var};
        $arr  = $+{var};
    }
    else { return ('', '', ''); }

    $type =~ s/^(unsigned|signed|struct)/$replace_type_prefix{$1}/x;
    $type =~ s/(const)/$replace_type_prefix{$1}/gx;
    $type =~ s/^(?:.?+)(bool)/$replace_type{lc $1}/ix;
    $type =~ s/^(uint|int)(?:\d\d_t)/$replace_type{lc $1}/ix;
    $type =~ s/(int|long|char|double)/$replace_type{$1}/x;
    $type =~ s{\s+}{}gx;

    $var =~ s{[^\w\d_]+}{}gx; #strip non conforming chars

    $arr =~ s{[^\[\d\]]+}{}gx;

    return ($type, $var, $arr);
}

sub Print_Variable {
    #prints the member, offset, size, and type from the assembly file
    my $sinput = $_[0];
    my ($member, $offset, $size, $type, $arr);

    $sinput =~ s{[^\w\d_,=\"\*]+}{}gx;
    if($sinput =~ $asmMOST_regex)
    {
        $member = $+{member};
        $offset = $+{offset};
        $size   = $+{size};
        $type = $+{type};
        $arr  = $+{arr};

        if($type =~ /^(.*)${array_marker}$/) #strip array marker add [n]
        {
            $type = sprintf('%s[%d]', $1, $arr);
        }

        printf "\t%s = {0x%x, %d, \"%s\"},\n", $member, $offset, $size, $type;
        return 1;
    }
    return 0;
}

if($header) #output sections to lua file
{
    print "-- Don't change this file!\n";
    printf "-- It is automatically generated of settings.h %s\n", $svnrev;
    print "-- member = {offset, size, \"type\"}\n\n";

    print "--";
    foreach my $key (sort(keys %replace_type_prefix)) {
        print $key, '= \'', $replace_type_prefix{$key}, '\', ';
    }
    print "\n--";
    foreach my $key (sort(keys %replace_type)) {
        print $key, '= \'', $replace_type{$key}, '\', ';
    }
    print "\n\n";

    for(my $i = 0; $i < @sections_lua; $i++)
    {
        print "$sections_lua[$i]$section_lua_suffix = {\n";

        my @members=split('@', $section_lists[$i]);
        $section_lists[$i] = '';

        foreach my $memb(@members)
        {
            $section_count[$i] -= Print_Variable($memb);
        }

        print "}\n\n";

        if($section_count[$i] ne 0)
        {
            warn "$0 Warning: Failed to extract '$sections[$i]'\n";
        }
    }
    my ($user,$system,$cuser,$csystem) = times;
    warn "Pass2 ".$user." ".$system." ".$cuser." ".$csystem."\n";
    exit;
}

#else output sections to .c file
my $header = join(", ", $helper_name, @sections);
my $print_variable = 'PRINT_M_O_S_T';
my $print_array    = 'PRINT_ARRAY_M_O_S_T';
my $emit_asm       = 'ASM_EMIT_M_O_S_T';

print <<EOF

#include <stddef.h> /* offsetof */
#include <stdbool.h>
#include "config.h"
#include "settings.h"

/* (ab)uses the compiler to emit member offset and size to asm comments */
/* GAS supports C-style comments in asm files other compilers may not */
/* "NAME", "MEMBER" = ?OFFSET, ?SIZE, "TYPE, ?ARRAYCT";
   NOTE: ? may differ between machines */

#undef ${emit_asm}
#define ${emit_asm}(name, member, type, offset, size, elems) asm volatile\\
("/* "#name ", " #member " = %0, %1, " #type ", %2; */\\n" : : \\
"n"(offset), "n"(size), "n"(elems))

#undef ${print_variable}
#define ${print_variable}(name, member, value, type) ${emit_asm}(#name, \\
#member, #type, offsetof(name, member), sizeof(value), 0)

#undef ${print_array}
#define ${print_array}(name, member, value, type) ${emit_asm}(#name, \\
#member, #type, offsetof(name, member), sizeof(value), sizeof(value)/sizeof(value[0]))

int main(void)
{

/* GAS supports C-style comments in asm files other compilers may not */
/* This header identifies assembler output for second pass */
asm volatile("/* $header; */");

EOF
;
my $section_prefix = "section_";
my $format_asm = "    %s(%s, %s, ${section_prefix}%d.%s, %s);\n";


for(my $i = 0; $i < @sections; $i++)
{
    printf "%s %s%d;\n", $sections[$i], $section_prefix, $i; #create variable for each section
    my @members=split('@', $section_lists[$i]);
    $section_lists[$i] = '';

    foreach my $memb(@members)
    {
        my ($type, $var, $arr) = Extract_Variable($memb);
        my $call = ${print_variable};
        if($type && $var)
        {
            if($type =~ /^.*${array_marker}$/){$call = ${print_array};}
            printf $format_asm, $call, $sections[$i], $var, $i, $var, $type;
        }
    }
}
print <<EOF

    return 0;
}

EOF
;

my ($user,$system,$cuser,$csystem) = times;
warn "Pass1 ".$user." ".$system." ".$cuser." ".$csystem."\n";
