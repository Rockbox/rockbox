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
use strict;
use warnings;

# Extracts members from c struct
# places them in a lua array 'member = {offset,size,"type"}'
# requires two passes
#  first pass outputs a c file that shall be compiled with the -S option
#  second pass extracts the member, offset, size, type from the assembly

my $svnrev = '$Revision$';
my $helper_name = 'LUA_RB_SETTINGS_H_HELPER';

############# configuration #############
my @sections = (
    '',
    'struct system_status',
    'struct user_settings',
    'struct replaygain_settings',
    'struct eq_band_setting',
    'struct compressor_settings',
    'struct mp3_enc_config',
    '', # nothing to be captured just placeholder for lua section
    'struct mp3entry',
    'struct mp3_albumart',
    'struct embedded_cuesheet',
    #'struct cuesheet',
    #'struct cue_track_info',
);

my @sections_lua = (
    'rb.system',
    'rb.system.global_status',
    'rb.system.global_settings',
    'rb.system.replaygain_settings',
    'rb.system.eq_band_setting',
    'rb.system.compressor_settings',
    'rb.system.mp3_enc_config',
    'rb.metadata',
    'rb.metadata.mp3_entry',
    'rb.metadata.mp3_albumart',
    'rb.metadata.embedded_cuesheet',
    #'rb.metadata.cuesheet',
    #'rb.metadata.cue_track_info',
);

# structs will have their dependencies included automagically
my @includes = ();

my $section_lua_suffix = '';

my %replace_type_prefix = (
    unsigned    => 'u_',
    signed      => '',
    struct      => 's_',
    const       => 'const_',
    enum        => 'e_',
    '#pointer#' => 'ptr_',
);

my %replace_type = (
    int         => 'i',
    uint        => 'u_i',
    long        => 'l',
    char        => 'c',
    bool        => 'b',
    double      => 'd',
    '#string#'  => 'str',
);

############# internal #############
my @section_count = ();  #variables found
my @section_found = ();  #{} bracket matches
my @section_lists = (); #variable declarations
my @section_asm_regex = ();
my @section_begin_regex = ();

my $header = '';
my $array_marker = '_typeisarray_';
my $pointer_marker = '_typeisptr_';
my $current_include;

############# precompile regex for speed #############
my $array_mark_regex   = qr/^.*${array_marker}$/;
my $pointer_mark_regex = qr/^(.*)${pointer_marker}.*$/;

my $incl_regex = qr/^.*\".+\/(.+\.h)\".*$/;
my $extern_regex = qr/^extern\b.*$/;

#type var ????;
my $decl_regex = qr/^(.+?\s.+;)/;
my $typevar_regex = qr/\W*(?<type>.*?)\W*(?<var>[^\s\[]+)(?<arr>)\W*;/;
my$typevar_array_regex = qr/\W*(?<type>.*?)\W*(?<var>[^\s\[]+)\W*(?<arr>\[.+\]).*;/;

# struct or union defined within another structure
my $embed_structunion_regex = qr/\W*(struct|union\s[^;}]+)/;

#.."section",.."member"..=..offset,..size,..type,..arrayct
my $asm_regex = qr/.*?,.*?(\".+?\".*?=.*?,.+?,.+?\".+\".*?,.*);/;
my $asmMOST_regex = qr/\"(?<member>.+)\"=(?<offset>\d+),(?<size>\d+),\"(?<type>.+)\",(?<arr>\d+).*/;

for(my $i = 0; $i < @sections; $i++)
{
    $section_asm_regex[$i] = qr/\"$sections[$i]\"${asm_regex}/;
    $section_begin_regex[$i] = qr/^$sections[$i]\b\s*+[^\*;]*$/;
    $section_count[$i] = 0;
    $section_found[$i] = 0;
    $section_lists[$i] = '';
}

my $section_end_regex  = qr/}\s*;.*$/;

#######################################################
#extract all the variables within the structs(sections)
#######################################################
while(my $line = <STDIN>)
{
    next if($line =~ /^\s+$/);

    chomp($line);

    if($header) # [PASS 2]
    {
        for(my $i = 0; $i < @sections; $i++)
        {
            next if(!$sections[$i]);

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
        for(my $i = 0; $i < @sections; $i++)
        {
            @section_lists[$i] = '';
            @section_count[$i] = 0;
        }
    }
    else # [PASS 1]
    {
        if($line =~ $incl_regex){$current_include = $1; next;}
        elsif($line =~ $extern_regex){next;}

        for(my $i = 0; $i < @sections; $i++)
        {
            next if(!$sections[$i]);

            if($section_found[$i] > 0)
            {
                # variable declaration?
                if($line =~ $decl_regex)
                {
                    $section_lists[$i] .= $1.'@';
                    $section_count[$i]++;
                }
                # struct or union defined within a structure skip if single line
                elsif($line =~ $embed_structunion_regex && (index($line, "}") < 0))
                {
                    my $embedtype = $line;
                    $embedtype =~ s{[^\w\d_]+}{}gx; #strip non conforming chars
                    while($line = <STDIN>)
                    {
                        #skip over the contents user will have to determine how to parse
                        if($line =~ /^\s*}.+$/)
                        {
                            if($line =~ $decl_regex)
                            {
                                $section_lists[$i] .= $embedtype." ".$1.'@';
                                $section_count[$i]++;
                            }
                            last
                        }
                    }
                }

                # struct end?
                if($line =~ $section_end_regex)
                {
                    $section_found[$i]--;
                }
                last;
            }
            elsif($line =~ $section_begin_regex[$i]) # struct begin?
            {
                if($current_include)
                {
                    push (@includes, $current_include);
                    $current_include = '';
                }
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
    #[PASS 1] extracts the member, offset, size, and type from the include file
    my $sinput = $_[0];
    my ($type, $var, $arr);

    $sinput =~ s{\s*\*\s*}{${pointer_marker}}gx;
    if($sinput =~ $typevar_array_regex) #arrays
    {
        $type = $+{type};
        $var  = $+{var};
        $arr  = $+{var};
        if($type eq ''){$type = "<unknown>";}
        if($sinput =~ s/\bchar\b//){$type = $replace_type{'#string#'};}
        else{$type .= ${array_marker};} #for identification of array .. stripped later
    }
    elsif($sinput =~ $typevar_regex)
    {
        $type = $+{type};
        $var  = $+{var};
        $arr  = $+{var};
    }
    else {
        warn "Skipping ".$sinput."\n";
        return ('', '', '');
    }

    # Type substitutions
    $type =~ s/^(unsigned|signed|struct)/$replace_type_prefix{$1}/x;
    $type =~ s/\b(const|enum)\b/$replace_type_prefix{$1}/gx;
    $type =~ s/^(?:.?+)(bool)\b/$replace_type{lc $1}/ix;
    $type =~ s/^(uint|int)(?:\d\d_t)\b/$replace_type{lc $1}/ix; # ...intNN_t...
    if($type =~ $array_mark_regex)
    {
        $type =~ s/\b(int|long|char|double)(${array_marker}.*)\b/$replace_type{$1}$2/;
    }
    else {$type =~ s/\b(int|long|char|double)\b/$replace_type{$1}/;}
    $type =~ s{\s+}{}gx;

    $var =~ s{[^\w\d_]+}{}gx; # strip non conforming chars

    $arr =~ s{[^\[\d\]]+}{}gx; # get element count

    return ($type, $var, $arr);
}

sub Print_Variable {
    #[PASS 2] prints the member, offset, size, and type from the assembly file
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

        if($type =~ /^(.*)${array_marker}$/) # strip array marker add [n]
        {
            $type = sprintf('%s[%d]', $1, $arr);
        }

        printf "\t%s = \"0x%x, %d, %s\",\n", $member, $offset, $size, $type;
        return 1;
    }
    return 0;
}

if($header) #output sections to lua file [PASS 2]
{
    print "-- Don't change this file!\n";
    printf "-- It is automatically generated %s\n", $svnrev;
    print "-- member = \"offset, size, type\"\n\n";

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
        if($sections_lua[$i])
        {
            print "$sections_lua[$i]$section_lua_suffix = {\n";

            my @members=split('@', $section_lists[$i]);
            $section_lists[$i] = '';

            foreach my $memb(@members)
            {
                $section_count[$i] -= Print_Variable($memb);
            }

            print "}\n\n";

            if($sections[$i] && $section_count[$i] ne 0)
            {
                warn "$0 Warning: Failed to extract '$sections[$i]'\n";
            }
        }
    }
    print "\nreturn false\n";
    #my ($user,$system,$cuser,$csystem) = times;
    #warn "Pass2 ".$user." ".$system." ".$cuser." ".$csystem."\n";
    exit;
}

#else output sections to .c file [PASS 1]
my $c_header = join(", ", $helper_name, @sections);
my $print_variable = 'PRINT_M_O_S_T';
my $print_array    = 'PRINT_ARRAY_M_O_S_T';
my $emit_asm       = 'ASM_EMIT_M_O_S_T';

foreach my $incl(@includes)
{
    printf "#include \"%s\"\n", $incl;
}

print <<EOF

#include <stddef.h> /* offsetof */
#include <stdbool.h>

/* (ab)uses the compiler to emit member offset and size to asm comments */
/* GAS supports C-style comments in asm files other compilers may not */
/* "NAME", "MEMBER" = ?OFFSET, ?SIZE, "TYPE, ?ARRAYCT";
   NOTE: ? may differ between machines */

#undef ${emit_asm}
#define ${emit_asm}(name, member, type, offset, size, elems) asm volatile\\
("/* "#name ", " #member " = %0, %1, " #type ", %2; */\\n" : : \\
"n"(offset), "n"(size), "n"(elems))

/* constraint 'n' - An immediate integer operand with a known numeric value is allowed */

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
asm volatile("/* $c_header; */");

EOF
;
my $section_prefix = "section_";
my $format_asm = "    %s(%s, %s, ${section_prefix}%d.%s, %s);\n";


for(my $i = 0; $i < @sections; $i++)
{
    if($sections[$i] && $section_lists[$i])
    {
        printf "%s %s%d;\n", $sections[$i], $section_prefix, $i; #create variable for each section
        my @members=split('@', $section_lists[$i]);
        $section_lists[$i] = '';

        foreach my $memb(@members)
        {
            my ($type, $var, $arr) = Extract_Variable($memb);
            my $call = ${print_variable};

            if($var =~ $pointer_mark_regex) #strip pointer marker
            {
                $type = $type.$1;
                $var =~ s{.*${pointer_marker}}{}gx;
                $type = $replace_type_prefix{'#pointer#'}.$type;
            }

            if($type && $var)
            {
                if($type =~ $array_mark_regex){$call = ${print_array};}
                printf $format_asm, $call, $sections[$i], $var, $i, $var, $type;
            }
        }
    }
}
print <<EOF

    return 0;
}

EOF
;

#my ($user,$system,$cuser,$csystem) = times;
#warn "Pass1 ".$user." ".$system." ".$cuser." ".$csystem."\n";
