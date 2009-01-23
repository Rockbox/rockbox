#!/usr/bin/perl
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

use strict;
use Getopt::Long qw(:config pass_through);	# pass_through so not confused by -DTYPE_STUFF

my $ROOT="..";
my $verbose;
my $rbdir=".rockbox";
my $wpslist;
my $target;

# Get options
GetOptions ( 'r|root=s'		=> \$ROOT,
	     'v|verbose'	=> \$verbose,
	     'rbdir=s'          => \$rbdir, # If we want to put in a different directory
    );

($wpslist, $target) = @ARGV;

my $firmdir="$ROOT/firmware";
my $cppdef = $target;
my @depthlist = ( 16, 8, 4, 2, 1 );

# These parameters are filled in as we parse wpslist
my $wps;
my $wps_prefix;
my $rwps;
my $width;
my $height;
my $font;
my $fgcolor;
my $bgcolor;
my $statusbar;
my $author;
my $req_g;
my $req_g_wps;
my $req_t_wps;
my $backdrop;
my $lineselectstart;
my $lineselectend;
my $selecttype;
my $iconset;
my $viewericon;
my $lineselecttextcolor;
my $filetylecolor;

# LCD sizes
my ($main_height, $main_width, $main_depth);
my ($remote_height, $remote_width, $remote_depth);
my $has_remote;


if(!$wpslist) {
    print "Usage: wpsbuilds.pl <WPSLIST> <target>\n",
    "Run this script in the root of the target build, and it will put all the\n",
    "stuff in $rbdir/wps/\n";
    exit;
}

sub getlcdsizes
{
    my ($remote) = @_;

    open(GCC, ">gcctemp");
    if($remote) {
        # Get the remote LCD screen size
    print GCC <<STOP
\#include "config.h"
#ifdef HAVE_REMOTE_LCD
Height: LCD_REMOTE_HEIGHT
Width: LCD_REMOTE_WIDTH
Depth: LCD_REMOTE_DEPTH
#endif
STOP
;
    }
    else {
    print GCC <<STOP
\#include "config.h"
Height: LCD_HEIGHT
Width: LCD_WIDTH
Depth: LCD_DEPTH
STOP
;
}
    close(GCC);

    my $c="cat gcctemp | gcc $cppdef -I. -I$firmdir/export -E -P -";

    #print "CMD $c\n";

    open(GETSIZE, "$c|");

    my ($height, $width, $depth);
    while(<GETSIZE>) {
        if($_ =~ /^Height: (\d*)/) {
            $height = $1;
        }
        elsif($_ =~ /^Width: (\d*)/) {
            $width = $1;
        }
        elsif($_ =~ /^Depth: (\d*)/) {
            $depth = $1;
        }
        if($height && $width && $depth) {
            last;
        }
    }
    close(GETSIZE);
    unlink("gcctemp");

    return ($height, $width, $depth);
}

sub mkdirs
{
    my $wpsdir = $wps;
    $wpsdir =~ s/\.(r|)wps//;
    mkdir "$rbdir/wps", 0777;
    mkdir "$rbdir/themes", 0777;

    if( -d "$rbdir/wps/$wpsdir") {
        #print STDERR "wpsbuild warning: directory wps/$wpsdir already exists!\n";
    }
    else
    {
       mkdir "$rbdir/wps/$wpsdir", 0777;
    }
}

sub copybackdrop
{
    #copy the backdrop file into the build dir
    if ($backdrop ne '') {
        my $dst = $backdrop;
        $dst =~ s/(\.[0-9]*x[0-9]*x[0-9]*)//;
        my $cmd = "cp $ROOT/$backdrop $rbdir/$dst";
        `$cmd`;
    }
}

sub copythemefont
{
    #copy the font specified by the theme

    my $o=$font;
    $o =~ s/\.fnt/\.bdf/;
    mkdir "$rbdir/fonts";
    my $cmd ="$ROOT/tools/convbdf -f -o \"$rbdir/fonts/$font\" \"$ROOT/fonts/$o\" ";
    `$cmd`;
}

sub copythemeicon
{
    #copy the icon specified by the theme

    if ($iconset ne '') {
        $iconset =~ s/.rockbox/$rbdir/;
        $iconset =~ /\/(.*icons\/(.*))/i;
        `cp $ROOT/icons/$2 $1`;
    }
}

sub copythemeviewericon
{
    #copy the viewer icon specified by the theme

    if ($viewericon ne '') {
        $viewericon =~ s/.rockbox/$rbdir/;
        $viewericon =~ /\/(.*icons\/(.*))/i;
        `cp $ROOT/icons/$2 $1`;
    }
}

sub copywps
{
    # we assume that we copy the WPS files from the same dir the WPSLIST
    # file is located in
    my $dir;
    my @filelist;
    my $file;

    if($wpslist =~ /(.*)WPSLIST/) {
        $dir = $1;
#        system("cp $dir/$wps .rockbox/wps/");
        #print "$req_t_wps $req_g_wps\n";

        if (-e "$dir/$req_t_wps" ) {
          system("cp $dir/$req_t_wps $rbdir/wps/$wps");

        } elsif (-e "$dir/$req_g_wps") {
           system("cp $dir/$req_g_wps $rbdir/wps/$wps");

           open(WPSFILE, "$dir/$req_g_wps");
           while (<WPSFILE>) {
              $filelist[$#filelist + 1] = $1 if (/\|([^|]*?.bmp)\|/);
           }
           close(WPSFILE);

           if ($#filelist >= 0) {
              if (-e "$dir/$wps_prefix/$req_g") {
                 foreach $file (@filelist) {
                     system("cp $dir/$wps_prefix/$req_g/$file $rbdir/wps/$wps_prefix/");
                 }
              }
              elsif (-e "$dir/$wps_prefix") {
                 foreach $file (@filelist) {
                     system("cp $dir/$wps_prefix/$file $rbdir/wps/$wps_prefix/");
                 }
              }
              else {
                  print STDERR "beep, no dir to copy WPS from!\n";
              }
           }

       } else {
           print STDERR "Skipping $wps - no matching resolution.\n";
       }
    } else {
        print STDERR "No source directory!\n";
    }
}

sub buildcfg {
    my $cfg = $wps;
    my @out;

    $cfg =~ s/\.(r|)wps/.cfg/;

    push @out, <<MOO
\#
\# $cfg generated by wpsbuild.pl
\# $wps is made by $author
\#
wps: /$rbdir/wps/$wps
MOO
;
    if($font) {
        push @out, "font: /$rbdir/fonts/$font\n";
    }
    if($fgcolor && $main_depth > 2) {
        push @out, "foreground color: $fgcolor\n";
    }
    if($bgcolor && $main_depth > 2) {
        push @out, "background color: $bgcolor\n";
    }
    if($statusbar) {
        push @out, "statusbar: $statusbar\n";
    }
    if(defined($backdrop)) {
        if ($backdrop eq '') {
            push @out, "backdrop:\n";
        } else {
            # clip resolution from filename
            $backdrop =~ s/(\.[0-9]*x[0-9]*x[0-9]*)//;
            push @out, "backdrop: /$rbdir/$backdrop\n";
        }
    }
    if($lineselectstart && $main_depth > 2) {
        push @out, "line selector start color: $lineselectstart\n";
    }
    if($lineselectend && $main_depth > 2) {
        push @out, "line selector end color: $lineselectend\n";
    }
    if($selecttype) {
        push @out, "selector type: $selecttype\n";
    }
    if(defined($iconset)) {
        push @out, "iconset: $iconset\n";
    }
    if(defined($viewericon)) {
        push @out, "viewers iconset: $viewericon\n";
    }
    if($lineselecttextcolor && $main_depth > 2 ) {
        push @out, "line selector text color: $lineselecttextcolor\n";
    }
    if($filetylecolor && $main_depth > 2) {
        push @out, "filetype colours: $filetylecolor\n";
    }
    if($rwps && $has_remote ) {
        push @out, "rwps: /$rbdir/wps/$rwps\n";
    }
    if(-f "$rbdir/wps/$cfg") {
        print STDERR "wpsbuild warning: wps/$cfg already exists!\n";
    }
    else {
        open(CFG, ">$rbdir/themes/$cfg");
        print CFG @out;
        close(CFG);
    }
}

# Get the LCD sizes first
($main_height, $main_width, $main_depth) = getlcdsizes();
($remote_height, $remote_width, $remote_depth) = getlcdsizes(1);

#print "LCD: ${main_width}x${main_height}x${main_depth}\n";
$has_remote = 1 if ($remote_height && $remote_width && $remote_depth);

my $isrwps;
my $within;

open(WPS, "<$wpslist");
while(<WPS>) {
    my $l = $_;
    
    # remove CR
    $l =~ s/\r//g;
    if($l =~ /^ *\#/) {
        # skip comment
        next;
    }
    if($l =~ /^ *<(r|)wps>/i) {
        $isrwps = $1;
        $within = 1;
        # undef is a unary operator (!)
        undef $wps;
        undef $wps_prefix;
        undef $rwps;
        undef $width;
        undef $height;
        undef $font;
        undef $fgcolor;
        undef $bgcolor;
        undef $statusbar;
        undef $author;
        undef $req_g_wps;
        undef $req_t_wps;
        undef $backdrop;
        undef $lineselectstart;
        undef $lineselectend;
        undef $selecttype;
        undef $iconset;
        undef $viewericon;
        undef $lineselecttextcolor;
        undef $filetylecolor;

        next;
    }
    if($within) {
        if($l =~ /^ *<\/${isrwps}wps>/i) {
            # Get the required width and height
            my ($rheight, $rwidth, $rdepth);
            if($isrwps) {
                ($rheight, $rwidth, $rdepth) =
                         ($remote_height, $remote_width, $remote_depth);
            }
            else {
                ($rheight, $rwidth, $rdepth) =
                         ($main_height, $main_width, $main_depth);
            }

            if(!$rheight || !$rwidth) {
                #printf STDERR "wpsbuild notice: No %sLCD size, skipping $wps\n",
                #$isrwps?"remote ":"";
                $within = 0;
                next;
            }
            $wpslist =~ /(.*)WPSLIST/;
            my $wpsdir = $1;
            # If this WPS installable on this platform, one of the following
            # two files will be present
            foreach my $d (@depthlist) {
                next if ($d > $rdepth);

                $req_g = $rwidth . "x" . $rheight . "x" . $d;

                $req_g_wps = $wps_prefix . "." . $req_g . ".wps";
                last if (-e "$wpsdir/$req_g_wps");

                if ($isrwps) {
                    $req_g = $req_g . "." . $main_width . "x" . $main_height . "x" . "$main_depth";

                    $req_g_wps = $wps_prefix . "." . $req_g . ".wps";
                    last if (-e "$wpsdir/$req_g_wps");
                }
            }
            $req_t_wps = $wps_prefix . ".txt" . ".wps";

            #print "LCD: $wps wants $width x $height\n";
            #print "LCD: is $rwidth x $rheight\n";

            #print "gwps: $wpsdir/$req_g_wps" . "\n";
            if (-e "$wpsdir/$req_g_wps" || -e "$wpsdir/$req_t_wps" ) {
                #
                # The target model has an LCD that is suitable for this
                # WPS
                #
                #print "Size requirement is fine!\n";
                mkdirs() if (-e "$wpsdir/$req_g_wps");
                # Do the copying before building the .cfg - buildcfg()
                # mangles some filenames
                if ($backdrop) {
                    copybackdrop();
                }
                if ($iconset) {
                    copythemeicon();
                }
                if ($viewericon) {
                    copythemeviewericon();
                }
                if ($font) {
                    copythemefont();
                }
                if(!$isrwps) {
                    # We only make .cfg files for <wps> sections:
                    buildcfg();
                }
                copywps();
            }
            else {
                #print "(${wps_prefix}-${rwidth}x${rheight}x$rdepth) ";
                #print "Skip $wps due to size restraints\n";
            }
            $within = 0;
        }
        elsif($l =~ /^Name: *(.*)/i) {
            # Note that in the case this is within <rwps>, $wps will contain the
            # name of the rwps. Use $isrwps to figure out what type it is.
            $wps = $wps_prefix = $1;
            $wps_prefix =~ s/\.(r|)wps//;
            #print $wps_prefix . "\n";
        }
        elsif($l =~ /^RWPS: *(.*)/i) {
            $rwps = $1;
        }
        elsif($l =~ /^RWPS\.${main_width}x${main_height}x$main_depth: *(.*)/i) {
            $rwps = $1;
        }
        elsif($l =~ /^Author: *(.*)/i) {
            $author = $1;
        }
        elsif($l =~ /^Width: *(.*)/i) {
            $width = $1;
        }
        elsif($l =~ /^Width\.${main_width}x${main_height}x$main_depth: *(.*)/i) {
            $width = $1;
        }
        elsif($l =~ /^Height: *(.*)/i) {
            $height = $1;
        }
        elsif($l =~ /^Height\.${main_width}x${main_height}x$main_depth: *(.*)/i) {
            $height = $1;
        }
        elsif($l =~ /^Font: *(.*)/i) {
            $font = $1;
        }
        elsif($l =~ /^Font\.${main_width}x${main_height}x$main_depth: *(.*)/i) {
            $font = $1;
        }
        elsif($l =~ /^Foreground Color: *(.*)/i) {
            $fgcolor = $1;
        }
        elsif($l =~ /^Background Color: *(.*)/i) {
            $bgcolor = $1;
        }
        elsif($l =~ /^Statusbar: *(.*)/i) {
            $statusbar = $1;
        }
        elsif($l =~ /^Statusbar\.${main_width}x${main_height}x$main_depth: *(.*)/i) {
            $statusbar = $1;
        }
        elsif($l =~ /^Backdrop: *(.*)/i) {
            $backdrop = $1;
        }
        elsif($l =~ /^Backdrop\.${main_width}x${main_height}x$main_depth: *(.*)/i) {
            $backdrop = $1;
        }
        elsif($l =~ /^line selector start color: *(.*)/i) {
            $lineselectstart = $1;
        }
        elsif($l =~ /^line selector end color: *(.*)/i) {
            $lineselectend = $1;
        }
        elsif($l =~ /^selector type: *(.*)/i) {
            $selecttype = $1;
        }
        elsif($l =~ /^selector type\.${main_width}x${main_height}x$main_depth: *(.*)/i) {
            $selecttype = $1;
        }
        elsif($l =~ /^iconset: *(.*)/i) {
            $iconset = $1;
        }
        elsif($l =~ /^iconset\.${main_width}x${main_height}x$main_depth: *(.*)/i) {
            $iconset = $1;
        }
        elsif($l =~ /^viewers iconset: *(.*)/i) {
            $viewericon = $1;
        }
        elsif($l =~ /^viewers iconset\.${main_width}x${main_height}x$main_depth: *(.*)/i) {
            $viewericon = $1;
        }
        elsif($l =~ /^line selector text color: *(.*)/i) {
            $lineselecttextcolor = $1;
        }
        elsif($l =~ /^filetype colours: *(.*)/i) {
            $filetylecolor = $1;
        }
        else{
            #print "Unknown line:  $l!\n";
        }
    }
}

close(WPS);
