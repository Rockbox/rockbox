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


# The purpose of this script is to automatically generate Lua wrappers for
# (easily) portable C functions used in the Rockbox plugin API.
# It doesn't contain support for structs or pointers (apart from char*).
#
# The output will be written to <build_dir>/apps/plugins/lua/rocklib_aux.c

sub trim
{
    my $text = $_[0];
    $text =~ s/^\s+//;
    $text =~ s/\s+$//;
    return $text;
}

sub rand_string
{
    my @chars=('a'..'z');
    my $ret;
    foreach (1..5) 
    {
        $ret .= $chars[rand @chars];
    }
    return $ret;
}


my @functions;
my @ported_functions;
# These functions are excluded from automatically wrapping. This is useful if
# you want to manually port them to Lua. The format is a standard Perl regular
# expression.
my @forbidden_functions = ('^atoi$',
                           '^open$',
                           '^open_utf8$',
                           '^close$',
                           'dcache',
                           '^read$',
                           '^write$',
                           '^mkdir$',
                           '^rmdir$',
                           '^remove$',
                           '^rename$',
                           '^lseek$',
                           '^ftruncate$',
                           '^filesize$',
                           '^fdprintf$',
                           '^read_line$',
                           '^[a-z]+dir$',
                           '^strip_extension$',
                           '^create_numbered_filename$',
                           '^s?+rand$',
                           '^strlen$',
                           '^strl?+cpy$',
                           '^strl?+cat$',
                           'strn?+casecmp$',
                           '^iso_decode$',
                           '^utf8encode$',
                           '^utf16',
                           '^codec_',
                           '^timer_',
                           '^lcd_blit_(mono|grey_phase)$',
                           '^lcd_(mono_)?+bitmap',
                           '^lcd_(draw|fill|update_)rect$',
                           '^lcd_draw(line|pixel)$',
                           '^lcd_(h|v)line$',
                           '^lcd_(update|clear_display|set_drawmode)$',
                           '^lcd_setfont$',
                           '^lcd_(set|get)_(fore|back)ground$',
                           '^lcd_put(s|sxy|s_scroll)$',
                           '^lcd_scroll_stop$',
                           '^backlight_o(n|ff)$',
                           '^backlight_set_brightness$',
                           '^buttonlight_set_brightness$',
                           '^__.+$',
                           '^.+_(un)?cached$',
                           '^audio_(status|get_file_pos|flush_and_reload_tracks)$',
                           '^audio_(ff_rewind|next|prev|play|pause|resume|stop)$',
                           '^playlist_(amount|add|create|start|resume|shuffle)$',
                           '^playlist_(sync|resume_track|remove_all_tracks)$',
                           '^playlist_(insert_track|insert_directory)$',
                           '^pcm_is_(playing|paused)$',
                           '^pcm_play_(stop|pause|lock|unlock)$',
                           '^pcm_(apply_settings|get_bytes_waiting)$',
                           '^pcm_(set_frequency|calculate_peaks)$',
                           '^sound_(set|current|default|min|max|unit|pitch|val2phys)$',
                           '^mixer_channel.*$',
                           '^mixer_(set|get)_frequency$',
                           '^plugin_get_current_filename$',
                           '^plugin_release_audio_buffer$',
                           '^reload_directory$',
                           '^settings_save$',
                           '^set_current_file$',
                           '^set_dirfilter$',
                           '^show_logo$',
                           '^sleep$',
                           '^system_memory_guard$',
                           '^system_sound_play$',
                           '^talk_number$',
                           '^talk_force_shutup$',
                           '^talk_spell$',
                           '^talk_shutup$',
                           '^(trigger|cancel)_cpu_boost$',
                           '^thread_',
                           '^touchscreen_(get|set)_mode$',
                           '^viewportmanager_theme_undo$',
                           '^round_value_to_list32$');

my $rocklib = sprintf("%s/rocklib.c", $ARGV[0]);
open ROCKLIB, "<$rocklib" or die("Couldn't open $rocklib: $!");
while(<ROCKLIB>)
{
    if(/^RB_WRAP\(([^)]+)\)/)
    {
        push(@ported_functions, $1);
    }
}
close ROCKLIB;

# Parse plugin.h
my $start = 0;
while(<STDIN>)
{
    if(/struct plugin_api \{/)
    {
        $start = 1;
    }
    elsif($start && /\};/)
    {
        $start = 0;
    }

    if($start == 1)
    {
        my $line = $_;
        while($line !~ /;/)
        {
            $line .= <STDIN>;
        }

        $line =~ s/(\n|\r)//g;

        if($line =~ /([a-zA-Z *_]+)?\s?\(\*([^)]+)?\)\(([^)]+)\).*?;/)
        {
            $return_type = $1;
            $name = $2;
            $arguments = $3;

            $return_type = trim($return_type);
            $arguments =~ s/\s{2,}/ /g;

            if( !grep($_ eq $name, @ported_functions) &&
                !grep($name =~ $_, @forbidden_functions))
            {
                push(@functions, {'name' => $name, 'return' => $return_type, 'arg' => $arguments});
            }
        }
    }
}

my $svnrev = '$Revision$';

# Print the header
print <<EOF
/* Automatically generated from rocklib.c & plugin.h ($svnrev)
 *
 * See apps/plugins/lua/rocklib_aux.pl for the generator.
 */

#define lrocklib_c
#define LUA_LIB

#define _ROCKCONF_H_ /* We don't need strcmp() etc. wrappers */
#include "lua.h"
#include "lauxlib.h"
#include "plugin.h"

EOF
;

my %in_types = ('void' => \&in_void,
                'enum' => \&in_enum,
                'int' => \&in_int,
                'unsigned' => \&in_int,
                'unsignedint' => \&in_int,
                'signed' => \&in_int,
                'signedint' => \&in_int,
                'short' => \&in_int,
                'unsignedshort' => \&in_int,
                'signedshort' => \&in_int,
                'long' => \&in_int,
                'unsignedlong' => \&in_int,
                'signedlong' => \&in_int,
                'char' => \&in_int,
                'unsignedchar' => \&in_int,
                'signedchar' => \&in_int,
                'size_t' => \&in_int,
                'ssize_t' => \&in_int,
                'off_t' => \&in_int,
                'char*' => \&in_string,
                'signedchar*' => \&in_string,
                'unsignedchar*' => \&in_string,
                'bool' => \&in_bool,
                '_Bool' => \&in_bool
), %out_types = ('void' => \&out_void,
                 'enum' => \&out_enum,
                 'int' => \&out_int,
                 'unsigned' => \&out_int,
                 'unsignedint' => \&out_int,
                 'signed' => \&out_int,
                 'signedint' => \&out_int,
                 'short' => \&out_int,
                 'unsignedshort' => \&out_int,
                 'signedshort' => \&out_int,
                 'long' => \&out_int,
                 'unsignedlong' => \&out_int,
                 'signedlong' => \&out_int,
                 'char' => \&out_int,
                 'unsignedchar' => \&out_int,
                 'signedchar' => \&out_int,
                 'size_t' => \&out_int,
                 'ssize_t' => \&out_int,
                 'off_t' => \&out_int,
                 'char*' => \&out_string,
                 'signedchar*' => \&out_string,
                 'unsignedchar*' => \&out_string,
                 'bool' => \&out_bool,
                 '_Bool' => \&out_bool
);

sub in_void
{
    return "\t(void)L;\n";
}

sub in_enum
{
    my ($name, $type, $pos) = @_;
    return sprintf("\t%s %s = luaL_checkint(L, %d); /*(enum)*/\n", "int", $name, $pos);
}

sub in_int
{
    my ($name, $type, $pos) = @_;
    return sprintf("\t%s %s = (%s) luaL_checkint(L, %d);\n", $type, $name, $type, $pos);
}

sub in_string
{
    my ($name, $type, $pos) = @_;
    return sprintf("\t%s %s = (%s) luaL_checkstring(L, %d);\n", $type, $name, $type, $pos)
}

sub in_bool
{
    my ($name, $type, $pos) = @_;
    return sprintf("\tbool %s = luaL_checkboolean(L, %d);\n", $name, $pos)
}

sub out_void
{
    my $name = $_[0];
    return sprintf("\t%s;\n\treturn 0;\n", $name);
}

sub out_enum
{
    my ($name, $type) = @_;
    return sprintf("\t%s result = %s;\n\tlua_pushinteger(L, result);\n\treturn 1; /*(enum)*/\n", "int", $name);
}

sub out_int
{
    my ($name, $type) = @_;
    return sprintf("\t%s result = %s;\n\tlua_pushinteger(L, result);\n\treturn 1;\n", $type, $name);
}

sub out_string
{
    my ($name, $type) = @_;
    return sprintf("\t%s result = %s;\n\tlua_pushstring(L, result);\n\treturn 1;\n", $type, $name);
}

sub out_bool
{
    my ($name, $type) = @_;
    return sprintf("\tbool result = %s;\n\tlua_pushboolean(L, result);\n\treturn 1;\n", $name);
}

#Sort the functions
my @sorted_functions = sort { @$a{'name'} cmp @$b{'name'} } @functions;

# Print the functions
my @valid_functions;
foreach my $function (@sorted_functions)
{
    my $valid = 1, @arguments = ();
    # Check for supported arguments
    foreach my $argument (split(/,/, @$function{'arg'}))
    {
        $argument = trim($argument);
        if($argument !~ /\[.+\]/ && ($argument =~ /^(.+[\s*])([^[*\s]*)/
                                     || $argument eq "void"))
        {
            my $literal_type, $type, $name;
            if($argument eq "void")
            {
                $literal_type = "void", $type = "void", $name = "";
            }
            else
            {
                $literal_type = trim($1), $name = trim($2), $type = trim($1);
                $type =~ s/^enum\s+.*/enum/g;
                $type =~ s/(\s|const)//g;

                if($name eq "")
                {
                    $name = rand_string();
                }
            }

            #printf "/* %s: %s|%s */\n", @$function{'name'}, $type, $name;
            if(!defined $in_types{$type})
            {
                $valid = 0;
                break;
            }

            push(@arguments, {'name' => $name,
                              'type' => $type,
                              'literal_type' => $literal_type
                             });
        }
        else
        {
            $valid = 0;
            break;
        }
    }

    # Check for supported return value
    my $return = @$function{'return'};
    $return =~ s/^enum\s+.*/enum/g;
    $return =~ s/(\s|const)//g;
    #printf "/* %s: %s [%d] */\n", @$function{'name'}, $return, $valid;
    if(!defined $out_types{$return})
    {
        $valid = 0;
    }

    if($valid == 1)
    {
        # Print the header
        printf "static int rock_%s(lua_State *L)\n".
            "{\n",
            @$function{'name'};

        # Print the arguments
        my $i = 1;
        foreach my $argument (@arguments)
        {
            print $in_types{@$argument{'type'}}->(@$argument{'name'}, @$argument{'literal_type'}, $i++);
        }

        # Generate the arguments string
        my $func_args = $arguments[0]{'name'};
        for(my $i = 1; $i < $#arguments + 1; $i++)
        {
            $func_args .= ", ".$arguments[$i]{'name'};
        }
        
        # Print the function call
        my $func = sprintf("rb->%s(%s)", @$function{'name'}, $func_args);

        # Print the footer
        print $out_types{$return}->($func, @$function{'return'});
        print "}\n\n";

        push(@valid_functions, $function);
    }
}

# Print the C array
print "const luaL_Reg rocklib_aux[] =\n{\n";
foreach my $function (@valid_functions)
{
    printf "\t{\"%s\", rock_%s},\n", @$function{'name'}, @$function{'name'};
}
print "\t{NULL, NULL}\n};\n\n";
