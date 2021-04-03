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
# Copyright (C) 2019 by William Wilgus
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
############################################################################

#rockbox to lua define generator, add names of constants to the array to include

if ($#ARGV + 1 != 1) {
    warn "no definition type defined";
    exit;
}

my $def_type = $ARGV[0];
#warn "$def_type\n";
my $lua_table;
my @rockbox_defines;

if ($def_type eq "rb_defines") {
    $lua_table = "rb";
    @rockbox_defines = (
        '^HZ$',
        '^LCD_(DEPTH|HEIGHT|WIDTH)$',
        '^MODEL_NAME$',
        '^SCREEN_MAIN$',
        '^LCD_DEFAULT_(FG|BG)$',
        '^LCD_REMOTE_(DEPTH|HEIGHT|WIDTH)$',
        '^LCD_.+(BRIGHT|DARK)COLOR',
        '^SCREEN_REMOTE$',
        '^FONT_SYSFIXED$',
        '^FONT_UI$',
        '^PLAYBACK_EVENT_.*',
        '^PLAYLIST_(INSERT|PREPEND|REPLACE)',
        '^TOUCHSCREEN_(POINT|BUTTON)$',
        '^SYS_CHARGER_(DIS|)CONNECTED$',
        '^SYS_(TIMEOUT|POWEROFF|BATTERY_UPDATE)$',
        '^SYS_USB_(DIS|)CONNECTED$',
        '^HOME_DIR$',
        '^PLUGIN(_OK|_USB_CONNECTED|_POWEROFF|_GOTO_WPS|_GOTO_PLUGIN)$',
        '^PLUGIN_DIR$',
        '^PLUGIN(_APPS_|_GAMES_|_)DATA_DIR$',
        '^ROCKBOX_DIR$',
        '^STYLE_(NONE|DEFAULT|INVERT|COLORBAR|GRADIENT|COLORED)',
        '^CORE_KEYMAP_FILE$',
        'CONTEXT_(STOPSEARCHING|REMOTE|CUSTOM|CUSTOM2|PLUGIN|REMAPPED)$',
        '^VIEWERS_DATA_DIR$');
}
elsif ($def_type eq "sound_defines") {
    $lua_table = "rb.sound_settings";
    @rockbox_defines = (
        '^(?!.*LAST_SETTING)SOUND_');
}

my @captured_defines;
my @names_seen;
my @all_defines;

############# precompile regex for speed #############
my $def_regex = qr/#define\s+([^\s\r\n]+)\s+([^\r\n]+)/;
my $quot_regex = qr/.*([\"\']).*/;
my $num_regex = qr/.*([\+\-\*\\|&\d]).*/;
my $configh_regex = qr/^\s*#define\s*__CONFIG_H__\s*$/;
my $config_h = "?";
my $exclude_regex = qr/^#define\s*_?POSIX.*/;
my $exclude_enum_regex = qr/^#define\s*(_SC_|_PC_|_CS_).*/;
print <<EOF
#include <stdio.h>
#include <stdbool.h>

#define stringify(s) #s

/* auto generated defines */


EOF
;

while(my $line = <STDIN>)
{
        if($config_h eq "?" && $line =~ $configh_regex) { $config_h = $line; }
        next if($config_h eq "?"); #don't capture till we get to the config file
        next if($line =~ $exclude_regex);

        if($line =~ $def_regex) #does it begin with #define?
        {
            push(@all_defines, $line); #save all defines for possible ambiguous macros
            $name = $1;
            next if $name =~ /^(ATTRIBUTE_|print|__).*/; #don't add reserved
            $value = $2;

            if(grep($name =~ $_, @rockbox_defines))
            {
                push(@names_seen, $name);
                push(@captured_defines, {'name' => $name, 'value' => $value});
                print $line
            }
            else
            {
                $name =~ s{(\(.*\))}{}gx; #remove (...) on function type macros
                #ifndef guard so we don't clash with the host
                printf "#ifndef %s\n%s#endif\n\n", $name, $line;
            }
        }
        elsif($line =~ /^(?!.*;)enum.*{/) #enum {
        {
            next if($line =~ /enum\s*__.*/); #don't add reserved
            print $line;
            next if($line =~ /.*};.*/);
            do_enum($line)
        }
        elsif($line =~ /^(?!.*[;\(\)])enum.*/) #enum
        {
            next if($line =~ /enum\s*__.*/); #don't add reserved
            #need to be careful might be a function returning enum
            my $lastline = $line;
            next if($line =~ /.*};.*/);
            ($line = <STDIN>);
            if($line =~ /.*{.*/)
            {
                print $lastline;
                print $line;
            }
            else { next; }
            do_enum($line)
        }
        elsif($line =~ /^enum.*{[^;]+};.*/) #enum {
        {
            next if($line =~ /enum\s*__.*/); #don't add reserved
            next if(do_enum($line));
            
        }
}
#warn "total defines: ".scalar @all_defines;
#warn "captured defines: ".scalar @captured_defines;
#Sort the functions
my @sorted_defines = sort { @$a{'name'} cmp @$b{'name'} } @captured_defines;

printf "int main(void)\n{\n";
printf "\tprintf(\"--[[Autogenerated rockbox constants]]\\n\\n\");\n\n";
printf "\tprintf(\"%s = %s or {}\\n\");\n", $lua_table, $lua_table;
# Print the C array
foreach my $define (@sorted_defines)
{
    if(@$define{'value'} =~ /^0[xX][0-9a-fA-F]+$/) #hex number
    {
        printf "\tprintf(\"%s[\\\"%%s\\\"] = 0x%%x\\n\", stringify(%s), %s);\n", $lua_table, @$define{'name'}, @$define{'name'};
    }
    elsif(@$define{'value'} =~ /^[0-9]+$/) #number
    {
        printf "\tprintf(\"%s[\\\"%%s\\\"] = %%d\\n\", stringify(%s), %s);\n", $lua_table, @$define{'name'}, @$define{'name'};
    }
    else #might be a string but we don't know since the macro isn't expanded far enough
    {
        my $max_depth = 10;
        my $var = @$define{'value'};
        while($max_depth > 0) #follow the chain of #defines until we can see what type
        {
            $max_depth--;
            $var = "\\\s*#define\\s+$var\\s+.*";
            #warn $var;
            if(my ($x) = grep($_ =~ /($var)/, @all_defines)) #check all the defines
            {
                #warn $x;
                if($x =~ $def_regex)
                {
                    $var = $2;
                    #warn $var;
                    last if ($var =~ $quot_regex);
                    last if ($var =~ $num_regex);
                    next
                }
                #warn "end ".$var;
                last
            }
            else
            {
                $var = @$define{'value'};
                last
            }
        }
        if ($var =~$quot_regex) #has a quote it is a string
        {
            #guard with empty literals "" so gcc throws an error if it isn't a string
            printf "\tprintf(\"%s[\\\"%%s\\\"] = \\\"%%s\\\"\\n\", stringify(%s), \"\" %s \"\");\n", $lua_table, @$define{'name'}, @$define{'name'};
        }
        elsif ($var =~$num_regex) #it must be a number
        {
            printf "\tprintf(\"%s[\\\"%%s\\\"] = %%d\\n\", stringify(%s), %s);\n", $lua_table, @$define{'name'}, @$define{'name'};
        }
        else { warn "Skipping ".@$define{'name'}." indeterminate macro type\n"; }
    }
}

print <<EOF
    return 0;
}

EOF
;

sub do_enum {
    my ($line) = @_;
    if($line =~ /.*enum.*{(.*)};.*/) #single line enums
    {
        print $line;
        $value = "0"; #enums are always integers
        my $enum = $1;
        $enum =~ s/\s+//g;;
        my @values = split(',', $enum);

        foreach my $name(@values) {
            if(grep($name =~ $_, @rockbox_defines))
            {
                push(@names_seen, $name);
                push(@captured_defines, {'name' => $name, 'value' => $value});
            }
        }
        return 1;
    }

    while($line = <STDIN>)
    {
        next if($line =~ $exclude_enum_regex);
        if($line =~ /.*};.*/)
        {
            print $line;
            last
        }
        elsif($line =~ /([^\s,\t]+)\s*=?.*,?/)
        {
            $name = $1;
            #printf "%s WHATTT?", $name;
            $value = "0"; #enums are always integers
        }
        print $line;
        if(grep($name =~ $_, @rockbox_defines))
        {
            push(@names_seen, $name);
            push(@captured_defines, {'name' => $name, 'value' => $value});
        }
        
    }
    return 0;
}
