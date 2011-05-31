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

# list of icons for strip
my @iconlist = (
    "mimetypes/audio-x-generic",                # Icon_Audio
    "places/folder",                            # Icon_Folder
    "actions/format-indent-more",               # Icon_Playlist
    "actions/media-playback-start",             # Icon_Cursor ###
    "apps/preferences-desktop-wallpaper",       # Icon_Wps
    "devices/computer",                         # Icon_Firmware ###
    "apps/preferences-desktop-font",            # Icon_Font
    "apps/preferences-desktop-locale",          # Icon_Language
    "categories/preferences-system",            # Icon_Config
    "status/software-update-available",         # Icon_Plugin
    "actions/bookmark-new",                     # Icon_Bookmark
    "places/start-here",                        # Icon_Preset
    "actions/go-jump",                          # Icon_Queued
    "actions/go-next",                          # Icon_Moving
    "devices/input-keyboard",                   # Icon_Keyboard
    "actions/mail-send-receive",                # Icon_Reverse_Cursor
    "apps/help-browser",                        # Icon_Questionmark
    "actions/document-properties",              # Icon_Menu_setting
    "categories/applications-other",            # Icon_Menu_functioncall
    "actions/list-add",                         # Icon_Submenu
    "categories/preferences-system",            # Icon_Submenu_Entered
    "actions/media-record",                     # Icon_Recording
    "devices/audio-input-microphone",           # Icon_Voice ###
    "categories/preferences-desktop",           # Icon_General_settings_menu
    "categories/applications-other",            # Icon_System_menu
    "actions/media-playback-start",             # Icon_Playback_menu
    "devices/video-display",                    # Icon_Display_menu
    "devices/video-display",                    # Icon_Remote_Display_menu
    "devices/network-wireless",                 # Icon_Radio_screen ###
    "mimetypes/package-x-generic",              # Icon_file_view_menu
    "apps/utilities-system-monitor",            # Icon_EQ
    "../rbutil/rbutilqt/icons/rockbox-clef.svg" # Icon_Rockbox
);


if($#ARGV < 1) {
    print "Usage: $0 <path to iconset> <size>\n";
    exit();
}
my $tangopath = $ARGV[0];
my $size = $ARGV[1];

# temporary files
my $alphatemp = File::Temp->new(SUFFIX => ".png");
my $alphatempfname = $alphatemp->filename();
my $exporttemp = File::Temp->new(SUFFIX => ".png");
my $exporttempfname = $exporttemp->filename();
my $tempstrip = File::Temp->new(SUFFIX => ".png");
my $tempstripfname = $tempstrip->filename();

my $newoutput = "tango_icons.$size.bmp";

if(-e $newoutput) {
    die("output file $newoutput does already exist!");
}

print "Creating icon strip as $newoutput\n\n";

my $count;
$count = 0;
foreach(@iconlist) {
    print "processing $_ ...\n";
    my $file;
    if(m/^$/) {
        # if nothing is defined make it empty / transparent
        my $s = $size . "x" . $size;
        `convert -size $s xc:"#f0f" $exporttempfname`
    }
    elsif(m/\.\./) {
        # icon is inside the Rockbox tree
        $file = $_;
        `inkscape --export-png=$exporttempfname --export-width=$size --export-height=$size $file`
    }
    else {
        # icon is inside the tango tree
        $file = "$tangopath/scalable/" . $_ . ".svg";
        `inkscape --export-png=$exporttempfname --export-width=$size --export-height=$size $file`
    }
    if($count != 0) {
        `convert -append $tempstripfname $exporttempfname $tempstripfname`;
    }
    else {
        `convert $exporttempfname $tempstripfname`;
    }
    $count++;
}
print "masking and converting result ...\n";
# create mask
`convert $tempstripfname -alpha extract -monochrome -negate -alpha copy -colorize 0,100,0 $alphatempfname`;
# combine mask with image and drop transparency and scale down
`convert -composite $tempstripfname $alphatempfname -flatten -background '#f0f' -alpha off $newoutput`;
print "done!\n";

