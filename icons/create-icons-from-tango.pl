#!/usr/bin/perl -w
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# Copyright (C) 2011 Dominik Riebeling
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.


# This script is to generate an iconset (iconstrip bmp file) from Tango icons.
# It should be usable for other iconsets that are provided as svg images. For
# those adjusting the paths to the icons might need adjustment.
# To be run from the icons/ folder in a Rockbox checkout.

use File::Temp;
use Getopt::Long;

# list of icons for strip
# Use relative paths to point to icons in the tree, relative to the icons/ folder
# (must start with . or ..)
# Other are treated relatively to the tango sources tree
my @iconlist = (
    "./tango-svg/audio-x-generic",                  # Icon_Audio
    "./tango-svg/folder",                           # Icon_Folder
    "./tango-svg/format-indent-more",               # Icon_Playlist
    "./new_icons-svg/media-playback-start-green",   # Icon_Cursor ###
    "./tango-svg/preferences-desktop-wallpaper",    # Icon_Wps
    "./tango-svg/media-flash",                      # Icon_Firmware ###
    "./tango-svg/preferences-desktop-font",         # Icon_Font
    "./tango-svg/preferences-desktop-locale",       # Icon_Language
    "./tango-svg/preferences-system",               # Icon_Config
    "./tango-svg/software-update-available",        # Icon_Plugin
    "./tango-svg/bookmark-new",                     # Icon_Bookmark
    "./tango-svg/start-here",                       # Icon_Preset
    "./tango-svg/go-jump",                          # Icon_Queued
    "./tango-svg/go-next",                          # Icon_Moving
    "./tango-svg/input-keyboard",                   # Icon_Keyboard
    "./tango-svg/go-previous",                      # Icon_Reverse_Cursor
    "./tango-svg/help-browser",                     # Icon_Questionmark
    "./tango-svg/document-properties",              # Icon_Menu_setting
    "./tango-svg/applications-other",               # Icon_Menu_functioncall
    "./tango-svg/list-add",                         # Icon_Submenu
    "./tango-svg/preferences-system",               # Icon_Submenu_Entered
    "./tango-svg/media-record",                     # Icon_Recording
    "./new_icons-svg/face-shout",                   # Icon_Voice ###
    "./tango-svg/preferences-desktop",              # Icon_General_settings_menu
    "./tango-svg/emblem-system",                    # Icon_System_menu
    "./tango-svg/media-playback-start",             # Icon_Playback_menu
    "./tango-svg/video-display",                    # Icon_Display_menu
    "./tango-svg/network-receive",                  # Icon_Remote_Display_menu
    "./tango-svg/network-wireless",                 # Icon_Radio_screen ###
    "./tango-svg/system-file-manager",              # Icon_file_view_menu
    "./tango-svg/utilities-system-monitor",         # Icon_EQ
    "../docs/logo/rockbox-clef"                     # Icon_Rockbox
);
my @iconlist_viewers = (
    "./tango-svg/applications-graphics",     # bmp
    "./tango-svg/applications-multimedia",   # mpeg
    "./tango-svg/applications-other",        # ch8, tap, sna, tzx, z80
    "./tango-svg/audio-x-generic",           # mp3, mid, rmi, wav
    "./tango-svg/format-justify-left",       # txt, nfo
    "./tango-svg/text-x-generic-template",   # ss
    "./mint-x-svg/applications-games",       # gb, gbc
    "./tango-svg/image-x-generic",           # jpg, jpe, jpeg
    "./tango-svg/format-indent-more",        # m3u
    "./mint-x-svg/gnome-glchess",            # pgn
    "./tango-svg/dialog-information",        # zzz
);


my $help;
my $do_viewers;
my $tangopath="";
my $size;
my @list = @iconlist;
my $output = "tango_icons";

GetOptions ( 'v'		=> \$do_viewers,
             't|tango-path=s' => \$tangopath,
             'o|output=s' => \$output,
             'h|help'   => \$help,
    );

if($#ARGV != 0 or $help) {
    print "Usage: $0 [-v] [-t <path to iconset>] [-o <output prefix>] SIZE\n";
    print "\n";
    print "\t-v\tGenerate viewer icons\n";
    print "\t-t\tPath to source of tango iconset if required\n";
    print "\t-t\tPath to source of tango iconset if required\n";
    print "\t-o\tUse <output prefix> instead of \"tango_icons\" for the output filename\n";
    print "\n";
    print "\tSIZE can be in the form of NN or NNxNN and will be used for the output filename\n";
    exit();
}


$size = $ARGV[0];

if ($do_viewers) {
    $output .= "_viewers";
    @list = @iconlist_viewers;
}
# temporary files
my $alphatemp = File::Temp->new(SUFFIX => ".png");
my $alphatempfname = $alphatemp->filename();
my $exporttemp = File::Temp->new(SUFFIX => ".png");
my $exporttempfname = $exporttemp->filename();
my $tempstrip = File::Temp->new(SUFFIX => ".png");
my $tempstripfname = $tempstrip->filename();

my $newoutput = "$output.$size.bmp";

if(-e $newoutput) {
    die("output file $newoutput does already exist!");
}

print "Creating icon strip as $newoutput\n\n";

my $count;
$count = 0;

foreach(@list) {
    print "processing $_ ...\n";
    my $file;
    if(m/^$/) {
        # if nothing is defined make it empty / transparent
        my $s = $size . "x" . $size;
        `convert -size $s xc:"#ff00ff" -alpha transparent $exporttempfname`
    }
    elsif(m/\./) {
        # icon is inside the Rockbox tree
        $file = $_ . ".svg";
        `inkscape --export-png=$exporttempfname --export-width=$size --export-height=$size $file`
    }
    else {
        # icon is inside the tango tree
        if ($tangopath eq "") {
            print "Path to tango sources needed but not given!\n";
            exit(1);
        }
        $file = "$tangopath/scalable/" . $_ . ".svg";
        #~ `inkscape --export-png=$exporttempfname --export-width=$size --export-height=$size $file`
        `cp $file tango-svg/`;
    }
    if($count != 0) {
        `convert -append $tempstripfname $exporttempfname $tempstripfname`;
    }
    else {
        `convert $exporttempfname $tempstripfname`;
    }
    $count++;
}
print "Converting result ...\n";
`convert $tempstripfname $newoutput`;
print "Done!\n";

