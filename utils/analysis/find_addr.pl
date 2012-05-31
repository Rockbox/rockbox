#!/usr/bin/env perl
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
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
use String::Scanf;
use Cwd;

sub check_boundaries
{
    my $fits = 0, $start = $_[0], $end = $_[0] + $_[1];
    foreach my $boundary (@ram_boundaries)
    {
        if(defined(@$boundary{'name'}) && $start >= @$boundary{'start'} && $end <= @$boundary{'end'})
        {
            return 1;
        }
    }

    return 0;
}

sub dynamic_space
{
    my $space = $_[0], $space_array = $_[1], $ret;

    printf "This address is in %s space, please select the %s which was used with this address:\n", $space, $space;
    $count = 1;
    foreach my $el (@$space_array)
    {
        printf " [%d]: %s\n", $count++, $el;
    }

    print "\n";
    my $sel = -1;
    do
    {
        print "Selection: ";
        $sel = <STDIN>;
    } while($sel <= 0 || $sel > $count - 1 || !($sel =~ /^[+-]?\d+$/));

    my $prefix;
    if($space eq 'plugin')
    {
        $prefix = 'apps';
    }
    else
    {
        $prefix = 'lib/rbcodec';
    }
    my $file = sprintf("%s/%ss/%s", $prefix, $space, @$space_array[$sel - 1]);
    $ret{'library'} = sprintf("%s/%s", cwd(), $file);
    open FILE, "$objdump -t $file |" or die "Can't open pipe: $!";
    while(<FILE>)
    {
        chomp($_);
        if(/^([0-9a-fA-F]+).+\s([0-9a-fA-F]{3,})\s(?:[^\s]+\s)?(.+)$/)
        {
            (my $addr) = sscanf("%lx", $1);
            (my $size) = sscanf("%lx", $2);

            if($lookaddr >= $addr && $lookaddr <= ($addr + $size))
            {
                my $diff = abs($lookaddr - $addr);
                if(!defined($ret{'diff'}) || $diff <= $ret{'diff'})
                {
                    $ret{'diff'} = $diff;
                    $ret{'function'} = $3;
                }
            }
        }
    }
    close FILE;

    return %ret;
}

($lookaddr) = sscanf("0x%lx", $ARGV[0]);
($context_size) = $#ARGV > 0 ? $ARGV[1] : 5;

if($lookaddr != 0)
{
    # Determine the used objdump utility
    open MAKEFILE, "<Makefile" or die "Can't open Makefile: $!";
    while(<MAKEFILE>)
    {
        chomp($_);

        if(/^export OC=(.+)$/)
        {
            $objdump = $1;
            $objdump =~ s/objcopy/objdump/;
        }
    }
    close MAKEFILE;

    # Generate a list of all codecs
    open FINDCODECS, "find lib/rbcodec/codecs/ -name '*.elf' 2>&1 |" or die "Can't open pipe: $!";
    my @codecs;
    while(<FINDCODECS>)
    {
        chomp($_);
        $_ =~ s/lib\/rbcodec\/codecs\///;
        push(@codecs, $_);
    }
    close FINDCODECS;
    # Generate a list of all plugins
    open FINDPLUGINS, "find apps/plugins/ -name '*.elf' 2>&1 |" or die "Can't open pipe: $!";
    my @plugins;
    while(<FINDPLUGINS>)
    {
        chomp($_);
        $_ =~ s/apps\/plugins\///;
        push(@plugins, $_);
    }
    close FINDPLUGINS;

    open MAPFILE, "<rockbox.map" or die "Can't open rockbox.map: $!";
    my $addr, $size, $library, $match, $prev_function, $codec_addr, $plugin_addr;
    while(<MAPFILE>)
    {
        chomp($_);

        if(/^\s*\.text\.([^\s]+)$/)
        {
            $prev_function = $1;
        }

        if(/^\.([^\s]+)\s*(0x[0-9a-fA-F]+)/)
        {
            ($addr) = sscanf("0x%lx", $2);
            if($1 eq "plugin")
            {
                $plugin_addr = $addr;
            }
            elsif($1 eq "codec")
            {
                $codec_addr = $addr;
            }
        }


        if(/^.*?\s*(0x[0-9a-fA-F]+)\s*(0x[0-9a-fA-F]+)\s(.+)$/)
        {
            ($addr)  = sscanf("0x%lx", $1);
            ($size)  = sscanf("0x%lx", $2);
            $library = $3;

            if(check_boundaries($addr, $size) != 0
               && $lookaddr >= $addr && $lookaddr <= ($addr + $size))
            {
                #printf "0x%x 0x%x %s %s\n", $addr, $size, $prev_function, $library;

                my $diff = abs($lookaddr - $addr);
                if(!defined($match{'diff'}) || $diff <= $match{'diff'})
                {
                    $match{'diff'} = $diff;
                    $match{'library'} = $library;
                    $match{'function'} = $prev_function;
                }
            }
        }
        elsif(/^\s*(0x[0-9a-fA-F]+)\s*([^\s]+)$/)
        {
            ($addr) = sscanf("0x%lx", $1);
            my $function = $2;

            if(check_boundaries($addr, 0) != 0 && $lookaddr >= $addr)
            {
                #printf "0x%x %s\n", $addr, $function;

                my $diff = abs($lookaddr - $addr);
                if(!defined($match{'diff'}) || $diff <= $match{'diff'})
                {
                    $match{'diff'} = $diff;
                    $match{'library'} = $library;
                    $match{'function'} = $function;
                }
            }
        }
        elsif(/^(.RAM) *(0x[0-9a-fA-F]+) (0x[0-9a-fA-F]+)/)
        {
            (my $start_addr) = sscanf("0x%lx", $2);
            (my $addr_length) = sscanf("0x%lx", $3);
            push(@ram_boundaries, {"name",  $1,
                                   "start", $start_addr,
                                   "end",   $start_addr + $addr_length
                                  });
        }
    }
    close MAPFILE;

    if($lookaddr >= $codec_addr && $lookaddr < $plugin_addr
       && $codec_addr != 0)
    {
        # look for codec
        %match = dynamic_space("codec", \@codecs);
    }
    elsif($lookaddr >= $plugin_addr && $plugin_addr != 0)
    {
        # look for plugin
        %match = dynamic_space("plugin", \@plugins);
    }

    printf "%s -> %s\n\n", $match{'library'}, $match{'function'};

    # Replace path/libfoo.a(bar.o) with path/libfoo.a
    $match{'library'} =~ s/\(.+\)//;

    open OBJDUMP, "$objdump -S $match{'library'} 2>&1 |" or die "Can't open pipe: $!";
    my $found = 0, $addr;
    while(<OBJDUMP>)
    {
        chomp($_);

        if(/^[0-9a-fA-F]+\s\<(.+)\>:$/)
        {
            $found = ($1 eq $match{'function'});
        }
        elsif(/Disassembly of section/)
        {
            $found = 0;
        }
        elsif($found == 1)
        {
            if(/^\s*([0-9a-fA-F]+):\s*[0-9a-fA-F]+\s*.+$/)
            {
                ($addr) = sscanf("%lx", $1);

                if($addr - $lookaddr > 0)
                {
                    $addr -= $lookaddr;
                }
                if(abs($match{'diff'} - $addr) <= $context_size * 4)
                {
                    printf "%s%s\n", ($addr == $match{'diff'} ? ">": " "), $_;
                }
            }
            else
            {
                # TODO: be able to also show source code (within context_size)
                # printf " %s\n", $_;
            }
        }
    }
    close OBJDUMP;
}
else
{
    print "find_addr.pl 0xABCDEF [CONTEXT_SIZE]\n\n";
    print <<EOF
This makes it possible to find the exact assembly instruction at the specified
memory location (depends on Makefile, rockbox.map and the object files).

Usage example:
    mcuelenaere\@wim2160:~/rockbox/build2\$ ../utils/analysis/find_addr.pl 0x8001a434 1
    /home/mcuelenaere/rockbox/build2/apps/screens.o -> id3_get_info

      23c:  00601021    move    v0,v1
    > 240:  80620000    lb  v0,0(v1)
      244:  0002180a    movz    v1,zero,v0


Don't forget to build with -g !
EOF
;
}
